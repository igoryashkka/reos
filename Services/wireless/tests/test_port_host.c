#include "rp_port.h"
#include "rp_port_host.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    assert(rp_port_now_ms() == 0);

    rp_port_host_set_now_ms(1000);
    assert(rp_port_now_ms() == 1000);

    rp_port_sleep_ms(50);
    assert(rp_port_now_ms() == 1050);

    rp_port_host_advance_ms(25);
    assert(rp_port_now_ms() == 1075);

    printf("ALL rp_port_host TESTS OK\n");
    return 0;
}
