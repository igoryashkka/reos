#include "spi.h"

int bsp_spi_init(bsp_spi_bus_id_t bus)
{
    (void)bus;

    return -1;
}

int bsp_spi_transfer(bsp_spi_bus_id_t bus, const uint8_t *tx, uint8_t *rx, size_t length)
{
    (void)bus;
    (void)tx;
    (void)rx;
    (void)length;

    return -1;
}

void bsp_spi_radio_reset(bsp_spi_bus_id_t bus)
{
    (void)bus;
}
