#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "utility.h"
#include "rx.h"

// These have special address / alignment requirements since they are used by DMA & WiFi hardware presumably?
// HACK: right now we put everything into the wifi sram
struct rx_dmadesc rx_dma_hdrdesc[13] __attribute__((aligned(16))) /*__attribute__((section(".wifi_ram")))*/;
struct rx_swdesc rx_swdesc_tab[13] __attribute__((aligned(16))) /*__attribute__((section(".wifi_ram")))*/;
struct rx_payloaddesc rx_payload_desc[13] __attribute__((aligned(16)))  /*__attribute__((section(".wifi_ram")))*/;
uint8_t rx_payload_desc_buffer[13*848] __attribute__((aligned(512))) /*__attribute__((section(".wifi_ram")))*/;

struct rxl_cntrl_env_tag {
    struct list ready;
    struct rx_dmadesc *first;
    struct rx_dmadesc *last;
    struct rx_dmadesc *free;
};
struct rxl_cntrl_env_tag rxl_cntrl_env;

struct rx_hwdesc_env_tag {
    struct rx_pbd* last;
    struct rx_pbd* free;
};
struct rx_hwdesc_env_tag rx_hwdesc_env;

struct rx_dmadesc * blmac_debug_rx_hdr_c_ptr_get() {
    return *(void**)0x44b08548;
}

void __attribute__ ((noinline)) blmac_dma_cntrl_set(uint32_t v) {
    *(volatile uint32_t*)0x44b08180 = v;
}

bool rx_any_ready() {
    return rxl_cntrl_env.ready.first != NULL;
}

void rxl_hd_append(struct rx_dmadesc *desc) {
    if (rxl_cntrl_env.free != blmac_debug_rx_hdr_c_ptr_get()) {
        struct rx_dmadesc* dfree = rxl_cntrl_env.free;
        rxl_cntrl_env.free = desc;
        desc = dfree;
    }
    desc->hd.next = NULL;
    desc->hd.first_pbd_ptr = NULL;
    desc->hd.statinfo.value = 0;
    desc->hd.frmlen = 0;
    rxl_cntrl_env.last->hd.next = desc;
    if (rxl_cntrl_env.first == NULL)
        rxl_cntrl_env.first = desc;
    rxl_cntrl_env.last = desc;
    // RX header new tail
    blmac_dma_cntrl_set(0x1000000);
}

struct rx_pbd * blmac_debug_rx_pay_c_ptr_get() {
    return *(void**)0x44b0854c;
}

void rxl_pd_append(struct rx_pbd* first, struct rx_pbd* last, struct rx_pbd* spare) {
    struct rx_pbd* free = rx_hwdesc_env.free;
    if (free == blmac_debug_rx_pay_c_ptr_get()) {
        free = first;
        if (last == NULL)
            free = spare;
        spare->bufstatinfo = 0;
    } else {
        struct rx_pbd* pbd = rx_hwdesc_env.free;
        if (last != NULL) {
            rx_hwdesc_env.free->next = first;
            pbd = last;
        }
        rx_hwdesc_env.free = spare;
        spare = pbd;
        free->bufstatinfo = 0;
    }
    spare->next = 0;
    rx_hwdesc_env.last->next = free;
    rx_hwdesc_env.last = spare;
    // RX payload new tail
    blmac_dma_cntrl_set(0x2000000);
}

void rxl_frame_release(struct rx_swdesc* desc) {
    struct rx_pbd* first = desc->dma_hdrdesc->hd.first_pbd_ptr;
    rxl_pd_append(first, desc->last_pbd, desc->spare_pbd);
    rxl_hd_append(desc->dma_hdrdesc);
}

void rxl_mpdu_free(struct rx_swdesc* desc) {
    struct rx_payloaddesc* pd = (struct rx_payloaddesc*) desc->dma_hdrdesc->hd.first_pbd_ptr;
    __asm volatile( "csrc mstatus, 8" );
    struct rx_payloaddesc* cur = NULL, *prev = NULL;
    do {
        cur = pd;
        cur->pd_status = 0;
        if (cur->pbd.bufstatinfo & 1) {
            desc->spare_pbd = &cur->pbd;
            desc->last_pbd = &prev->pbd;
            rxl_frame_release(desc);
            break;
        }
        prev = cur;
        pd = (struct rx_payloaddesc*) cur->pbd.next;
    } while (pd != NULL);
    __asm volatile( "csrs mstatus, 8" );
    return;
}

void rx_process(frame_handler handler) {
    do {
        struct rx_swdesc* desc = (struct rx_swdesc*)rxl_cntrl_env.ready.first;
        if (desc == NULL)
            break;

        __asm volatile( "csrc mstatus, 8" );
        rxl_cntrl_env.ready.first = desc->list_hdr.next;
        __asm volatile( "csrs mstatus, 8" );

        handler(desc);
        rxl_mpdu_free(desc);
    } while (true);
}

extern void blmac_tx_rx_int_ack_clear();

static void list_append(struct list* list, struct list_header* item) {
    if (list->first == NULL) {
        list->first = item;
    } else {
        list->last->next = item;
    }
    list->last = item;
    item->next = NULL;
}

void rx_timer_int_handler() {
    blmac_tx_rx_int_ack_clear();

    while (1) {
        struct rx_swdesc *swdesc;

        while (1) {
            if (rxl_cntrl_env.first == NULL || !rxl_cntrl_env.first->hd.statinfo.frm_successful_rx)
                return;

            swdesc = rxl_cntrl_env.first->hd.swdesc;
            struct rx_hd *hd = &rxl_cntrl_env.first->hd;
            struct rx_dmadesc *dmadesc = swdesc->dma_hdrdesc;
            rxl_cntrl_env.first = rxl_cntrl_env.first->hd.next;
            swdesc->pd = (struct rx_payloaddesc *)hd->first_pbd_ptr;
            if (dmadesc->hd.frmlen == 0)
                break;

            list_append(&rxl_cntrl_env.ready, &swdesc->list_hdr);
        }

        swdesc->spare_pbd = NULL;
        swdesc->last_pbd = NULL;
        rxl_hd_append(swdesc->dma_hdrdesc);
    }
}

void blmac_rx_header_head_ptr_set(struct rx_dmadesc *head) {
    *(volatile struct rx_dmadesc**)0x44b081b8 = head;
}

void blmac_rx_payload_head_ptr_set(struct rx_payloaddesc *head) {
    *(volatile struct rx_payloaddesc**)0x44b081bc = head;
}

void rx_init() {
    __asm volatile( "csrc mstatus, 8" );

    struct rx_dmadesc *d = NULL;
    memset(rx_dma_hdrdesc, 0, sizeof(rx_dma_hdrdesc));
    for (int i = 0; i < (sizeof(rx_dma_hdrdesc)/sizeof(struct rx_dmadesc)); i++) {
        d = rx_dma_hdrdesc + i;
        d->hd.upatternrx = 0xbaadf00d;
        d->hd.next = rx_dma_hdrdesc + i + 1;
        d->hd.swdesc = rx_swdesc_tab + i;
        rx_swdesc_tab[i].dma_hdrdesc = d;
    }
    d->hd.next = NULL;
    blmac_rx_header_head_ptr_set(&rx_dma_hdrdesc[1]);

    struct rx_payloaddesc *pd = NULL;
    memset(rx_payload_desc, 0, sizeof(rx_payload_desc));
    const int n = sizeof(rx_payload_desc)/sizeof(struct rx_payloaddesc);
    const int bufsize = sizeof(rx_payload_desc_buffer) / n;
    uint8_t* dataptr = rx_payload_desc_buffer;
    for (int i = 0; i < n; i++) {
        pd = rx_payload_desc + i;
        pd->pbd.upattern = 0xc0dedbad;
        pd->pbd.next = (struct rx_pbd*)(rx_payload_desc + i + 1);
        pd->pbd.datastartptr = dataptr;
        pd->pbd.dataendptr = dataptr + bufsize - 1;
        pd->buffer_rx = pd->pbd.datastartptr;
        dataptr += bufsize;
    }
    pd->pbd.next = NULL;
    blmac_rx_payload_head_ptr_set(&rx_payload_desc[1]);

    rxl_cntrl_env.first = &rx_dma_hdrdesc[1];
    rxl_cntrl_env.last = d;
    rxl_cntrl_env.free = &rx_dma_hdrdesc[0];
    rx_hwdesc_env.last = (struct rx_pbd*)pd;
    rx_hwdesc_env.free = (struct rx_pbd*)(&rx_payload_desc[0]);

    rxl_cntrl_env.ready.first = rxl_cntrl_env.ready.last = NULL;

    // signal RX header new head
    blmac_dma_cntrl_set(0x4000000);
    // signal RX payload new head
    blmac_dma_cntrl_set(0x8000000);
    __asm volatile( "csrs mstatus, 8" );
}
