#ifndef RE_INTC
#define RE_INTC

#include <stdint.h>
#include <stdbool.h>

#define WIFI_IRQ (16+54)
#define BLE_IRQ  (16+56)

void intc_init();
bool intc_is_pending(int num);
void intc_clear_pending(int num);

#endif
