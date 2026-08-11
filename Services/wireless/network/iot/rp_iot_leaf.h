#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Діагностичні геттери, специфічні для ролі leaf — поза rp_network.h, бо
 * не спільні з router/gateway. Дійсні лише коли лінкується rp_iot_leaf.c. */
uint32_t rp_iot_leaf_reading_drops(void);
uint8_t  rp_iot_leaf_duty_percent(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
