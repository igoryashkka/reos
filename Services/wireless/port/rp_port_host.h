#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Керування симульованим годинником rp_port_host.c. Використовується
 * лише з tests/wireless/ — шар roles бачить час виключно через rp_port.h. */
void rp_port_host_set_now_ms(uint32_t ms);
void rp_port_host_advance_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif
