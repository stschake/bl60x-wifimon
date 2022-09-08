#include <stdint.h>
#include <stdio.h>

#include "mac.h"
#include "rx.h"

bool intc_is_pending(int num) {
    return *(volatile uint8_t*)(0x2800000 + num);
}

void intc_clear_pending(int num) {
    *(volatile uint8_t*)(0x2800000 + num) = 0;
}

static int intc_irq_index_get() {
    return *(volatile int*)0x44910040;
}

static int intc_irq_status_get() {
    int ret = *(volatile int*)0x44910000;
    ret |= *(volatile int*)0x44910004;
    return ret;
}

static void intc_enable_irq(int index) {
    *((volatile uint32_t*)0x44910010 + (index >> 5)) = 1 << (index & 0x1F);
}

void intc_init() {
    intc_enable_irq(50);
    intc_enable_irq(52);
    intc_enable_irq(54);
}

void mac_irq() {
    if (intc_irq_status_get() == 0)
        return;

    // only handling the ones we expect
    int index = intc_irq_index_get();
    switch (index) {
    case 54:
        mac_gen_handler();
        break;
    case 52:
    case 50:
        rx_timer_int_handler();
        break;

    default:
        break;
    }
}
