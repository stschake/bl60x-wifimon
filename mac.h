#ifndef RE_MAC
#define RE_MAC

#include <stdint.h>

void mac_irq();
void mac_init();
void mac_active();
void mac_monitor_mode();
void mac_gen_handler();

#endif
