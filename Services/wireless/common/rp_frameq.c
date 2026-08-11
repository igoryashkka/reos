#include "rp_frameq.h"

#include <string.h>

void rp_frameq_init(rp_frameq_t *q, uint16_t capacity)
{
    if (!q) return;

    if (capacity > RP_FRAMEQ_MAX_DEPTH)
    {
        capacity = RP_FRAMEQ_MAX_DEPTH;
    }

    memset(q->items, 0, sizeof(q->items));
    q->capacity = capacity;
    q->head     = 0u;
    q->count    = 0u;
}

int rp_frameq_push(rp_frameq_t *q, const rp_frame_t *f)
{
    if (!q || !f) return -1;
    if (f->tail_len > RP_MAX_TAIL) return -1;
    if (q->count >= q->capacity) return -1;

    uint16_t tail_idx = (uint16_t)((q->head + q->count) % q->capacity);
    rp_frameq_item_t *item = &q->items[tail_idx];

    item->hdr      = f->hdr;
    item->core     = f->core;
    item->tail_len = f->tail_len;
    item->rssi     = f->rssi;
    item->snr      = f->snr;

    if (f->tail_len && f->tail)
    {
        memcpy(item->tail, f->tail, f->tail_len);
    }

    q->count++;

    return 0;
}

int rp_frameq_pop(rp_frameq_t *q, rp_frameq_item_t *out)
{
    if (!q || !out) return 0;
    if (q->count == 0u) return 0;

    *out = q->items[q->head];
    q->head = (uint16_t)((q->head + 1u) % q->capacity);
    q->count--;

    return 1;
}

int rp_frameq_is_empty(const rp_frameq_t *q)
{
    return !q || (q->count == 0u);
}

int rp_frameq_is_full(const rp_frameq_t *q)
{
    return q && (q->count >= q->capacity);
}
