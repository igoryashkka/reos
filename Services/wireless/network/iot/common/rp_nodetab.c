#include "rp_nodetab.h"

#include <string.h>

void rp_nodetab_init(rp_nodetab_t *t)
{
    if (!t) return;

    memset(t->entries, 0, sizeof(t->entries));
}

rp_node_entry_t *rp_nodetab_find_by_addr(rp_nodetab_t *t, uint16_t short_addr)
{
    if (!t) return NULL;

    for (uint16_t i = 0; i < RP_MAX_NODES; i++)
    {
        if (t->entries[i].in_use && (t->entries[i].short_addr == short_addr))
        {
            return &t->entries[i];
        }
    }

    return NULL;
}

rp_node_entry_t *rp_nodetab_find_by_dev_id(rp_nodetab_t *t, const uint8_t dev_id[RP_DEV_ID_LEN])
{
    if (!t || !dev_id) return NULL;

    for (uint16_t i = 0; i < RP_MAX_NODES; i++)
    {
        if (t->entries[i].in_use && (memcmp(t->entries[i].dev_id, dev_id, RP_DEV_ID_LEN) == 0))
        {
            return &t->entries[i];
        }
    }

    return NULL;
}

rp_node_entry_t *rp_nodetab_register(
    rp_nodetab_t *t,
    const uint8_t dev_id[RP_DEV_ID_LEN],
    uint16_t short_addr,
    uint32_t session_id,
    uint32_t now_ms,
    uint32_t keepalive_timeout_ms
)
{
    if (!t || !dev_id) return NULL;

    rp_node_entry_t *e = rp_nodetab_find_by_dev_id(t, dev_id);

    if (!e)
    {
        for (uint16_t i = 0; i < RP_MAX_NODES; i++)
        {
            if (!t->entries[i].in_use)
            {
                e = &t->entries[i];
                break;
            }
        }
    }

    if (!e) return NULL;

    memset(e, 0, sizeof(*e));
    e->in_use     = 1u;
    memcpy(e->dev_id, dev_id, RP_DEV_ID_LEN);
    e->short_addr = short_addr;
    e->session_id = session_id;
    e->seq_valid  = 0u;

    rp_nodetab_refresh_keepalive(e, now_ms, keepalive_timeout_ms);

    return e;
}

void rp_nodetab_remove(rp_nodetab_t *t, rp_node_entry_t *e)
{
    (void)t;

    if (!e) return;

    memset(e, 0, sizeof(*e));
}

void rp_nodetab_expire(rp_nodetab_t *t, uint32_t now_ms)
{
    if (!t) return;

    for (uint16_t i = 0; i < RP_MAX_NODES; i++)
    {
        rp_node_entry_t *e = &t->entries[i];

        if (e->in_use && (now_ms >= e->keepalive_deadline_ms))
        {
            rp_nodetab_remove(t, e);
        }
    }
}

void rp_nodetab_refresh_keepalive(rp_node_entry_t *e, uint32_t now_ms, uint32_t keepalive_timeout_ms)
{
    if (!e) return;

    e->keepalive_deadline_ms = now_ms + keepalive_timeout_ms;
}

int rp_nodetab_is_dup_seq(const rp_node_entry_t *e, uint8_t seq)
{
    if (!e) return 0;

    return e->seq_valid && (e->last_seq == seq);
}

void rp_nodetab_touch_seq(rp_node_entry_t *e, uint8_t seq)
{
    if (!e) return;

    e->last_seq  = seq;
    e->seq_valid = 1u;
}
