#include "rp_hw_if.h"
#include "logger.h"

static int rp_hw_radio_init(void)
{
    LOG_INFO(LOG_MODULE_RFM, "radio hw interface initialized (stub, no driver wired yet)");

    return 0;
}

static int rp_hw_radio_send(const uint8_t *data, size_t length)
{
    (void)data;

    LOG_WARNING(LOG_MODULE_RFM, "radio send not implemented, dropped len=%u", (unsigned)length);

    return -1;
}

const rp_hw_if_t rp_hw_if_radio =
{
    .name = "radio",

    .init = rp_hw_radio_init,

    .send = rp_hw_radio_send
};
