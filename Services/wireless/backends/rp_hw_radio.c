#include "rp_hw_if.h"
#include "logger.h"
#include "rfm66.h"

static rfm66_t g_rfm66;

static int rp_hw_radio_init(void)
{
    rfm66_config_t cfg = {
        .frequency_hz = 868000000u,
        .bitrate_bps  = 19200u,
    };

    if (rfm66_init(&g_rfm66, &cfg) != RFM66_OK)
    {
        LOG_WARNING(LOG_MODULE_RFM, "rfm66_init failed (no SPI bus wired on this board?)");
        return -1;
    }

    LOG_INFO(LOG_MODULE_RFM, "radio hw interface initialized (rfm66)");

    return 0;
}

static int rp_hw_radio_send(const uint8_t *data, size_t length)
{
    if (rfm66_transmit(&g_rfm66, data, (uint16_t)length) != RFM66_OK)
    {
        LOG_WARNING(LOG_MODULE_RFM, "radio send failed len=%u", (unsigned)length);
        return -1;
    }

    return (int)length;
}

const rp_hw_if_t rp_hw_if_radio =
{
    .name = "radio",

    .init = rp_hw_radio_init,

    .send = rp_hw_radio_send
};
