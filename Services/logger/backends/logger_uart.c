#include "logger_backend.h"
#include "logger_format.h"
#include "uart.h"

#include <stdio.h>

static int logger_uart_init(void)
{
    return uart_init();
}


static int logger_uart_write(
    const logger_record_t *record
)
{
    char buffer[256];
    int length = logger_format_record(buffer, sizeof(buffer), record);

    if (length <= 0)
    {
        return -1;
    }

    if ((size_t)length >= sizeof(buffer))
    {
        length = (int)(sizeof(buffer) - 1U);
    }

    return uart_write(
        (uint8_t *)buffer,
        length
    );
}


static int logger_uart_flush(void)
{
    return uart_flush();
}


const logger_backend_t logger_uart_backend =
{
    .name = "uart",

    .init = logger_uart_init,

    .write = logger_uart_write,

    .flush = logger_uart_flush
};