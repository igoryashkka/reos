#include "spi.h"

#include "config_pin.h"
#include "main.h"

/* Радіо-шина = SPI1 (hspi1): SCK/MISO/MOSI = PA5/PA6/PA7 (налаштовує
 * HAL_SPI_MspInit(hspi1) у stm32f4xx_hal_msp.c), CS = PA4 програмний
 * (SPI_NSS_SOFT — bsp_spi_transfer сама дьоргає BSP_RFM66_CS_*), RESET
 * = PC4 */
extern SPI_HandleTypeDef hspi1;

int bsp_spi_init(bsp_spi_bus_id_t bus)
{
    if (bus != BSP_SPI_BUS_RADIO) return -1;

    HAL_GPIO_WritePin(BSP_RFM66_CS_PORT, BSP_RFM66_CS_PIN, GPIO_PIN_SET); /* idle = high */

    return 0; /* MX_SPI1_Init() уже виконано в main() */
}

int bsp_spi_transfer(bsp_spi_bus_id_t bus, const uint8_t *tx, uint8_t *rx, size_t length)
{
    if (bus != BSP_SPI_BUS_RADIO) return -1;
    if ((tx == NULL) || (length == 0u)) return -1;

    uint8_t scratch[32];
    uint8_t *rx_buf = rx;

    if (rx_buf == NULL)
    {
        if (length > sizeof(scratch)) return -1;
        rx_buf = scratch;
    }

    HAL_GPIO_WritePin(BSP_RFM66_CS_PORT, BSP_RFM66_CS_PIN, GPIO_PIN_RESET);

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        &hspi1, (uint8_t *)tx, rx_buf, (uint16_t)length, HAL_MAX_DELAY
    );

    HAL_GPIO_WritePin(BSP_RFM66_CS_PORT, BSP_RFM66_CS_PIN, GPIO_PIN_SET);

    return (status == HAL_OK) ? (int)length : -1;
}

void bsp_spi_radio_reset(bsp_spi_bus_id_t bus)
{
    if (bus != BSP_SPI_BUS_RADIO) return;

    /* SX1231-родина: RESET активний ВИСОКИМ рівнем, >=100us; чип готовий
     * приймати SPI не раніше ніж за ~5ms після відпускання (даташит). */
    HAL_GPIO_WritePin(BSP_RFM66_RESET_PORT, BSP_RFM66_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(BSP_RFM66_RESET_PORT, BSP_RFM66_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(5);
}
