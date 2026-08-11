#include "rp_route.h"

#include <string.h>

void rp_route_init(rp_route_table_t *t)
{
    if (!t) return;

    memset(t->entries, 0, sizeof(t->entries));
}

rp_route_t *rp_route_find(rp_route_table_t *t, uint16_t destination, uint32_t now_ms)
{
    if (!t) return NULL;

    for (uint16_t i = 0; i < RP_P2P_MAX_ROUTES; i++)
    {
        rp_route_t *r = &t->entries[i];

        if (r->in_use && (r->destination == destination))
        {
            if (now_ms >= r->expires_ms) return NULL;
            return r;
        }
    }

    return NULL;
}

rp_route_t *rp_route_upsert(
    rp_route_table_t *t,
    uint16_t destination,
    uint16_t next_hop,
    uint16_t metric,
    uint32_t now_ms,
    uint32_t ttl_ms
)
{
    if (!t) return NULL;

    rp_route_t *r = NULL;

    for (uint16_t i = 0; i < RP_P2P_MAX_ROUTES; i++)
    {
        if (t->entries[i].in_use && (t->entries[i].destination == destination))
        {
            r = &t->entries[i];
            break;
        }
    }

    if (r)
    {
        int expired       = (now_ms >= r->expires_ms);
        int same_next_hop = (r->next_hop == next_hop);
        int better        = (metric < r->metric);

        if (!expired && !same_next_hop && !better)
        {
            return r; /* існуючий маршрут кращий чи рівний — лишаємо */
        }

        r->next_hop   = next_hop;
        r->metric     = metric;
        r->expires_ms = now_ms + ttl_ms;

        return r;
    }

    r = &t->entries[0];
    for (uint16_t i = 1; i < RP_P2P_MAX_ROUTES; i++)
    {
        if (!t->entries[i].in_use)
        {
            r = &t->entries[i];
            break;
        }
        if (t->entries[i].expires_ms < r->expires_ms)
        {
            r = &t->entries[i];
        }
    }

    r->in_use       = 1u;
    r->destination  = destination;
    r->next_hop     = next_hop;
    r->metric       = metric;
    r->expires_ms   = now_ms + ttl_ms;

    return r;
}

void rp_route_expire(rp_route_table_t *t, uint32_t now_ms)
{
    if (!t) return;

    for (uint16_t i = 0; i < RP_P2P_MAX_ROUTES; i++)
    {
        if (t->entries[i].in_use && (now_ms >= t->entries[i].expires_ms))
        {
            memset(&t->entries[i], 0, sizeof(t->entries[i]));
        }
    }
}

void rp_route_remove(rp_route_table_t *t, rp_route_t *r)
{
    (void)t;

    if (!r) return;

    memset(r, 0, sizeof(*r));
}

uint16_t rp_route_metric_default(const rp_neighbor_t *via_neighbor, uint16_t hop_count)
{
    if (!via_neighbor) return 0xFFFFu;

    uint8_t  quality      = rp_neighbor_quality(via_neighbor); /* 0..100, вище = краще */
    uint16_t quality_cost = (uint16_t)(100u - quality);        /* 0..100, нижче = краще */

    return (uint16_t)((uint32_t)hop_count * 10u + quality_cost);
}
