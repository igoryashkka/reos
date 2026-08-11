#pragma once

#include "rp_msg.h"
#include "rp_nodetab.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Один pending-downlink на вузол, індексований так само, як rp_nodetab
 * (індекс entries[i] у nodetab == індекс slots[i] тут). Кодування в кадр
 * відкладене до моменту фактичної доставки у вікні вузла, а не тут. */
typedef struct {
    uint8_t  pending;
    rp_msg_t msg;
} rp_mailbox_slot_t;

typedef struct {
    rp_mailbox_slot_t slots[RP_MAX_NODES];
} rp_mailbox_t;

void rp_mailbox_init(rp_mailbox_t *mb);

/* latest-wins: новий rp_mailbox_put() перезаписує ще недоставлений pending */
int  rp_mailbox_put(rp_mailbox_t *mb, uint16_t index, const rp_msg_t *msg);

int  rp_mailbox_has_pending(const rp_mailbox_t *mb, uint16_t index);

/* копіює у out_msg і знімає pending; 1 якщо було що брати, 0 якщо порожньо */
int  rp_mailbox_take(rp_mailbox_t *mb, uint16_t index, rp_msg_t *out_msg);

void rp_mailbox_clear(rp_mailbox_t *mb, uint16_t index);

#ifdef __cplusplus
}
#endif
