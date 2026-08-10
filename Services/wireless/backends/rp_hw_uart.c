#include "rp_hw_if.h"
#include "uart.h"

static int rp_hw_uart_init(void)
{
    return uart_init();
}

static int rp_hw_uart_send(const uint8_t *data, size_t length)
{
    return uart_write(data, length);
}

const rp_hw_if_t rp_hw_if_uart =
{
    .name = "uart",

    .init = rp_hw_uart_init,

    .send = rp_hw_uart_send
};
