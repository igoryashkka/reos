#include "uart.h"

#include "main.h"

/* huart5 конфігурується через MX_UART5_Init() у main.c (HAL, а не сирі
 * регістри, як для H750VB) — тут лише реалізація контракту uart.h
 * поверх уже готового handle. */
extern UART_HandleTypeDef huart5;

int uart_init(void)
{
    return 0;
}

int uart_write(const uint8_t *data, size_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return 0;
    }

    if (HAL_UART_Transmit(&huart5, (uint8_t *)data, (uint16_t)length, HAL_MAX_DELAY) != HAL_OK)
    {
        return -1;
    }

    return (int)length;
}

int uart_flush(void)
{
    return 0;
}
