#pragma once

#include "rp_msg.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Адресований downlink на конкретний вузол за короткою адресою.
 * rp_network_submit() з rp_network.h не несе адресата — контракт
 * спільний з leaf/router/P2P, де його й немає сенсу додавати. Повертає
 * RP_MSG_E_ARG, якщо вузол не зареєстрований. Кладе в rp_mailbox —
 * фактична передача відбувається опортуністично, коли вузол дає про
 * себе знати (tick()). */
int rp_iot_gateway_queue_downlink(uint16_t short_addr, const rp_msg_t *msg);

#ifdef __cplusplus
}
#endif
