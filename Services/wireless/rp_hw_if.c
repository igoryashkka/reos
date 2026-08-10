#include "rp_hw_if.h"

extern const rp_hw_if_t rp_hw_if_uart;
extern const rp_hw_if_t rp_hw_if_radio;

static const rp_hw_if_t *interfaces[RP_HW_IF_COUNT] =
{
    [RP_HW_IF_UART]  = &rp_hw_if_uart,
    [RP_HW_IF_RADIO] = &rp_hw_if_radio
};

static rp_hw_if_id_t active_if = RP_HW_IF_UART;

void rp_hw_if_init_all(void)
{
    for (size_t i = 0; i < RP_HW_IF_COUNT; i++)
    {
        if (interfaces[i]->init)
        {
            interfaces[i]->init();
        }
    }
}

int rp_hw_if_select(rp_hw_if_id_t id)
{
    if (id >= RP_HW_IF_COUNT)
    {
        return -1;
    }

    active_if = id;

    return 0;
}

int rp_hw_if_send(const uint8_t *data, size_t length)
{
    const rp_hw_if_t *hw_if = interfaces[active_if];

    if ((hw_if == NULL) || (hw_if->send == NULL))
    {
        return -1;
    }

    return hw_if->send(data, length);
}
