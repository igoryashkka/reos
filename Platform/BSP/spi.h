#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Портативний контракт SPI-шини — той самий підхід, що й uart.h:
 * заголовок тут, реалізація per-board (Boards/<board>/Core/Src/spi_*.c).
 * Драйвери чипів (Drivers/rfm66/) знають лише цей контракт і НІКОЛИ не
 * бачать SPI_HandleTypeDef/HAL напряму — це й робить їх портативними.
 *
 * bsp_spi_transfer() — ОДНА атомарна транзакція: сама смикає CS
 * (assert перед, deassert після) навколо повнодуплексного обміну.
 * Драйверу чипа не потрібно знати ні порт/пін CS, ні його таймінги. */
typedef enum {
    BSP_SPI_BUS_RADIO = 0, /* шина, до якої підключено RFM66 */
    BSP_SPI_BUS_COUNT
} bsp_spi_bus_id_t;

int bsp_spi_init(bsp_spi_bus_id_t bus);

/* Full-duplex: rx може бути NULL, якщо відповідь не потрібна.
 * Повертає length при успіху, <0 при помилці. */
int bsp_spi_transfer(bsp_spi_bus_id_t bus, const uint8_t *tx, uint8_t *rx, size_t length);

/* Апаратний reset чипа на цій шині (якщо RST розведений і відомий) —
 * блокуюча, сама витримує потрібні затримки. No-op на платах без
 * підтвердженого RST-піна (див. per-board реалізацію). */
void bsp_spi_radio_reset(bsp_spi_bus_id_t bus);

#ifdef __cplusplus
}
#endif
