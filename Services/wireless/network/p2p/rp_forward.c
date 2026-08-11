#include "rp_forward.h"

#include <string.h>

void rp_dup_cache_init(rp_dup_cache_t *c)
{
    if (!c) return;

    memset(c, 0, sizeof(*c));
}

int rp_dup_cache_check_and_add(rp_dup_cache_t *c, uint16_t src, uint8_t seq, uint32_t now_ms)
{
    if (!c) return 0;

    for (uint16_t i = 0; i < RP_P2P_DUP_CACHE_SIZE; i++)
    {
        rp_dup_entry_t *e = &c->entries[i];

        if (e->in_use && (e->src == src) && (e->seq == seq)
            && ((now_ms - e->seen_ms) < RP_P2P_DUP_WINDOW_MS))
        {
            e->seen_ms = now_ms; /* освіжаємо вікно */
            return 1;
        }
    }

    rp_dup_entry_t *slot = &c->entries[c->next_evict];
    slot->in_use  = 1u;
    slot->src     = src;
    slot->seq     = seq;
    slot->seen_ms = now_ms;

    c->next_evict = (uint16_t)((c->next_evict + 1u) % RP_P2P_DUP_CACHE_SIZE);

    return 0;
}

int rp_forward_rebuild(uint8_t *out, size_t out_cap, const rp_frame_t *f, int8_t *out_ttl_after)
{
    if (!out || !f) return -1;
    if (f->hdr.frag == 0u) return -1; /* TTL вичерпано (ТЗ §15) */

    rp_tx_meta_t meta;
    memset(&meta, 0, sizeof(meta));
    meta.type  = f->hdr.type;
    meta.flags = f->hdr.flags;
    meta.seq   = f->hdr.seq;
    meta.src   = f->hdr.src;
    meta.dst   = f->hdr.dst;
    meta.frag  = (uint8_t)(f->hdr.frag - 1u);

    if (out_ttl_after)
    {
        *out_ttl_after = (int8_t)meta.frag;
    }

    return rp_build(out, out_cap, &meta, &f->core, f->tail_len ? f->tail : NULL, f->tail_len);
}
