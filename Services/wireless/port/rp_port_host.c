//#include "rp_port.h"
#include "rp_port_host.h"

static uint32_t g_now_ms;

uint32_t rp_port_now_ms(void)
{
    return g_now_ms;
}

void rp_port_sleep_ms(uint32_t ms)
{
    g_now_ms += ms;
}

void rp_port_host_set_now_ms(uint32_t ms)
{
    g_now_ms = ms;
}

void rp_port_host_advance_ms(uint32_t ms)
{
    g_now_ms += ms;
}
