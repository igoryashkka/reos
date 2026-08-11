#pragma once

#include "rp_msg.h"
#include "rp_neighbor.h"
#include "rp_network.h"
#include "rp_route.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Адресована відправка на конкретний вузол мережі. rp_network_submit()
 * з rp_network.h не несе адресата — той самий compromise, що й
 * rp_iot_gateway_queue_downlink() для IoT gateway (rp_iot_gateway.h):
 * контракт спільний з IoT-ролями, де адресат або не потрібен (leaf), або
 * несе інший сенс (gateway short_addr), тож у фіксовану сигнатуру його
 * додавати не варто.
 *
 * Неблокуюче: ставить у чергу до RP_P2P_PENDING_TX_DEPTH відправлень;
 * фактичний вибір шляху (прямий сусід/маршрут/flood-фолбек, ТЗ §11)
 * відбувається в rp_network_tick(), де є now_ms. */
int rp_p2p_submit_to(uint16_t destination, const rp_msg_t *msg, rp_network_prio_t prio);

/* Діагностика/спостережуваність (логи, DEV_INFO, тести) — поза
 * фіксованим контрактом rp_network.h, як і відповідні геттери IoT-ролей
 * (rp_iot_leaf.h/rp_iot_gateway.h). NULL, якщо запису немає. */
const rp_neighbor_t *rp_p2p_find_neighbor(uint16_t addr);
const rp_route_t    *rp_p2p_find_route(uint16_t addr, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
