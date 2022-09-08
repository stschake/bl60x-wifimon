#include <stdint.h>
#include <stdbool.h>

#include "utility.h"
#include "intc.h"
#include "phy.h"
#include "mac.h"
#include "rx.h"

#include "board.h"

static inline uint16_t pbd_len(struct rx_pbd* pbd) {
    return (uint16_t)(pbd->dataendptr - pbd->datastartptr + 1);
}

void frame_rx_handler(struct rx_swdesc* swdesc) {
    uint16_t frame_len = swdesc->dma_hdrdesc->hd.frmlen;
    struct rx_pbd* pbd = swdesc->dma_hdrdesc->hd.first_pbd_ptr;
    struct rx_hd* hd = &swdesc->dma_hdrdesc->hd;

    board_toggle_led();
    //printf("< frame len %x rssi %d\r\n", frame_len, hd->recvec1c.rssi1);
    // this is not fully correct - there should be frames requiring multiple pbds,
    // but we are not really seeing them?
    int len = pbd_len(pbd);
    if (frame_len < len)
        len = frame_len;
    hexdump(pbd->datastartptr, len);
    board_send_uart('\r');
    board_send_uart('\n');
}

int main() {
    // setup board & 2 Mbaud uart
    board_init(2*1000*1000);

    rfc_init();
    intc_init();
    mac_init();
    rx_init();
    phy_init();
    phy_hw_set_channel(2437);
    mac_monitor_mode();
    mac_active();

    while (1) {
        if (intc_is_pending(WIFI_IRQ)) {
            intc_clear_pending(WIFI_IRQ);
            mac_irq();
        }

        if (rx_any_ready())
            rx_process(frame_rx_handler);
    }

    return 0;
}
