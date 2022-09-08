#include "board.h"

#include <bl602_glb.h>
#include <bl602_uart.h>
#include <stdbool.h>

void board_init(int baud) {
    GLB_GPIO_Cfg_Type gpio_cfg;
    gpio_cfg.gpioFun = GPIO_FUN_GPIO;
    gpio_cfg.gpioPin = 14;
    gpio_cfg.drive = 0;
    gpio_cfg.smtCtrl = 1;
    gpio_cfg.gpioMode = GPIO_MODE_OUTPUT;
    gpio_cfg.pullType = GPIO_PULL_NONE;
    GLB_GPIO_Init(&gpio_cfg);

    UART_CFG_Type uart_cfg =
    {
        (160*1000*1000)/4,
        baud,
        UART_DATABITS_8,
        UART_STOPBITS_1,
        UART_PARITY_NONE,
        DISABLE,
        DISABLE,
        DISABLE,
        UART_LSB_FIRST
    };
    UART_FifoCfg_Type fifo_cfg =
    {
        0x10,
        0x10,
        DISABLE,
        DISABLE
    };
    GLB_Set_UART_CLK(1, HBN_UART_CLK_160M, 3);
    GLB_GPIO_Cfg_Type uart_gpio;
    uart_gpio.drive = 1;
    uart_gpio.smtCtrl = 1;
    uart_gpio.gpioFun = 7;
    uart_gpio.gpioPin = 7;
    uart_gpio.gpioMode = GPIO_MODE_AF;
    uart_gpio.pullType = GPIO_PULL_UP;
    GLB_GPIO_Init(&uart_gpio);
    uart_gpio.gpioPin = 16;
    uart_gpio.gpioMode = GPIO_MODE_AF;
    uart_gpio.pullType = GPIO_PULL_UP;
    GLB_GPIO_Init(&uart_gpio);
    GLB_UART_Fun_Sel(7%8, GLB_UART_SIG_FUN_UART0_RXD);
    GLB_UART_Fun_Sel(16%8, GLB_UART_SIG_FUN_UART0_TXD);
    UART_IntMask(0, UART_INT_ALL, MASK);
    UART_Disable(0, UART_TXRX);
    UART_Init(0, &uart_cfg);
    UART_TxFreeRun(0, ENABLE);
    UART_FifoConfig(0, &fifo_cfg);
    UART_Enable(0, UART_TXRX);
}

void board_toggle_led() {
    static bool state = false;

    GLB_GPIO_Write(14, !!state);
    state = !state;
}

void board_send_uart(uint8_t c) {
    while (UART_GetTxFifoCount(0) == 0) {
    }

    BL_WR_BYTE(UART0_BASE + UART_FIFO_WDATA_OFFSET, c);
}
