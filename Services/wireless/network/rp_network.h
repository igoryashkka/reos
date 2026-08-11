#pragma once

#include "rp_hw_if.h"
#include "rp_msg.h"
#include "rp_proto.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t addr;
    uint16_t gateway_addr;      /* IoT leaf/router; ігнорується для IoT gateway й P2P */
    uint8_t  dev_id[RP_DEV_ID_LEN];
    uint16_t hw_ver;
    uint32_t fw_ver;
    uint32_t caps;
    rp_hw_if_id_t hw_if;
} rp_network_cfg_t;

typedef enum { RP_NETWORK_PRIO_NORMAL = 0, RP_NETWORK_PRIO_URGENT = 1 } rp_network_prio_t;

/* Єдиний API, який бачить App/. Реалізується рівно одним файлом,
 * обраним на етапі збірки через RP_NETWORK_MODEL (rp_config.cmake):
 *   RP_NETWORK_MODEL=P2P -> network/p2p/rp_p2p.c
 *   RP_NETWORK_MODEL=IOT -> network/iot/rp_iot_<RP_IOT_ROLE>.c
 *
 * now_ms — єдине джерело часу; платформні виклики (xTaskGetTickCount,
 * HAL_GetTick, vTaskDelay) заборонені всередині network/, common/, port/
 * — лише через port/rp_port.h. App/ не знає, яка модель/роль обрана. */

void     rp_network_init(const rp_network_cfg_t *cfg);

/* неблокуюче; повертає RP_MSG_E_* (rp_msg.h) при переповненні/помилці */
int      rp_network_submit(const rp_msg_t *msg, rp_network_prio_t prio);

/* викликається з rx-шляху після CRC-валідації (rp_parse_byte() == 1) */
void     rp_network_on_frame(const rp_frame_t *frame, uint32_t now_ms);

/* уся періодична робота: ретраї, keepalive/forwarding, доставка */
void     rp_network_tick(uint32_t now_ms);

/* скільки МС можна безпечно спати; 0 = негайно є робота */
uint32_t rp_network_next_deadline_ms(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
