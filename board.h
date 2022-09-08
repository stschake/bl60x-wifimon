#ifndef RE_BOARD
#define RE_BOARD

#include <stdint.h>

void board_init(int baud);
void board_toggle_led();
void board_send_uart(uint8_t c);

#endif
