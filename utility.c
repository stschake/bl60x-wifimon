#include "utility.h"
#include "board.h"

#include <ctype.h>

void hexdump(void *mem, unsigned int len)
{
    const int cols = 16;
    const char* hex = "0123456789ABCDEF";
    unsigned int i, j;

    for (i = 0; i < len + ((len % cols) ? (cols - len % cols) : 0); i++) {
        /* print hex data */
        if (i < len) {
            uint8_t v = ((uint8_t*)mem)[i];
            board_send_uart(hex[(v >> 4) & 0xf]);
            board_send_uart(hex[v & 0xf]);
            board_send_uart(' ');
        } else {
            /* end of block, just aligning for ASCII dump */
            board_send_uart(' ');
            board_send_uart(' ');
            board_send_uart(' ');
        }

        /* print ASCII dump */
        if (i % cols == (cols - 1)) {
            for (j = i - (cols - 1); j <= i; j++) {
                if (j >= len) {
                    /* end of block, not really printing */
                    board_send_uart(' ');
                } else if(isprint((int)((char*)mem)[j])) {
                    /* printable char */
                    board_send_uart(0xFF & ((char*)mem)[j]);
                } else {
                    /* other char */
                    board_send_uart('.');
                }
            }
            board_send_uart('\r');
            board_send_uart('\n');
        }
    }
}
 
