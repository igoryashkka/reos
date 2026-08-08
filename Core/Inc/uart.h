#pragma once

#include <stddef.h>
#include <stdint.h>

int uart_init(void);
int uart_write(const uint8_t *data, size_t length);
int uart_flush(void);