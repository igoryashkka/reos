#pragma once

#include "rp_msg.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RP_MAX_NODES
#define RP_MAX_NODES 16u
#endif

typedef struct {
    uint8_t  in_use;
    uint8_t  dev_id[RP_DEV_ID_LEN];
    uint16_t short_addr;
    uint32_t session_id;
    uint8_t  last_seq;
    uint8_t  seq_valid;       /* 0 доки не прийшов перший кадр після реєстрації */
    uint32_t next_wake_ms;    /* очікуване вікно прийому вузла, керує rp_role_gateway */
    uint32_t keepalive_deadline_ms;
} rp_node_entry_t;

typedef struct {
    rp_node_entry_t entries[RP_MAX_NODES];
} rp_nodetab_t;

void rp_nodetab_init(rp_nodetab_t *t);

rp_node_entry_t *rp_nodetab_find_by_addr(rp_nodetab_t *t, uint16_t short_addr);
rp_node_entry_t *rp_nodetab_find_by_dev_id(rp_nodetab_t *t, const uint8_t dev_id[RP_DEV_ID_LEN]);

/* Повторна реєстрація того ж dev_id оновлює існуючий запис на місці.
 * Інакше займає перший вільний слот. NULL, якщо таблиця повна. */
rp_node_entry_t *rp_nodetab_register(
    rp_nodetab_t *t,
    const uint8_t dev_id[RP_DEV_ID_LEN],
    uint16_t short_addr,
    uint32_t session_id,
    uint32_t now_ms,
    uint32_t keepalive_timeout_ms
);

void rp_nodetab_remove(rp_nodetab_t *t, rp_node_entry_t *e);

/* Видаляє всі записи з простроченим keepalive_deadline_ms; звільнене
 * місце може бути перевикористане наступною rp_nodetab_register(). */
void rp_nodetab_expire(rp_nodetab_t *t, uint32_t now_ms);

void rp_nodetab_refresh_keepalive(rp_node_entry_t *e, uint32_t now_ms, uint32_t keepalive_timeout_ms);

/* дедуп за (short_addr, seq): перевірка БЕЗ оновлення */
int  rp_nodetab_is_dup_seq(const rp_node_entry_t *e, uint8_t seq);
void rp_nodetab_touch_seq(rp_node_entry_t *e, uint8_t seq);

#ifdef __cplusplus
}
#endif
