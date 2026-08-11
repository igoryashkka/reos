#include "rp_neighbor.h"

#include <string.h>

void rp_neighbor_init(rp_neighbor_table_t *t)
{
    if (!t) return;

    memset(t->entries, 0, sizeof(t->entries));
}

rp_neighbor_t *rp_neighbor_find(rp_neighbor_table_t *t, uint16_t addr)
{
    if (!t) return NULL;

    for (uint16_t i = 0; i < RP_P2P_MAX_NEIGHBORS; i++)
    {
        if (t->entries[i].in_use && (t->entries[i].addr == addr))
        {
            return &t->entries[i];
        }
    }

    return NULL;
}

rp_neighbor_t *rp_neighbor_touch(rp_neighbor_table_t *t, uint16_t addr, uint32_t now_ms, int16_t rssi, int16_t snr)
{
    if (!t) return NULL;

    rp_neighbor_t *n = rp_neighbor_find(t, addr);

    if (!n)
    {
        for (uint16_t i = 0; i < RP_P2P_MAX_NEIGHBORS; i++)
        {
            if (!t->entries[i].in_use)
            {
                n = &t->entries[i];
                break;
            }
        }
    }

    if (!n)
    {
        n = &t->entries[0];
        for (uint16_t i = 1; i < RP_P2P_MAX_NEIGHBORS; i++)
        {
            if (t->entries[i].last_seen_ms < n->last_seen_ms)
            {
                n = &t->entries[i];
            }
        }
    }

    if (!n->in_use || (n->addr != addr))
    {
        memset(n, 0, sizeof(*n));
        n->addr = addr;
    }

    n->in_use       = 1u;
    n->rssi         = rssi;
    n->snr          = snr;
    n->last_seen_ms = now_ms;

    return n;
}

void rp_neighbor_note_latency(rp_neighbor_t *n, uint16_t latency_ms)
{
    if (!n) return;

    if (n->latency_ms == 0u)
    {
        n->latency_ms = latency_ms;
        return;
    }

    /* проста EMA (вага нового зразка 1/4), щоб один викид не смикав метрику */
    n->latency_ms = (uint16_t)(((uint32_t)n->latency_ms * 3u + latency_ms) / 4u);
}

void rp_neighbor_note_delivery(rp_neighbor_t *n, int delivered)
{
    if (!n) return;

    uint8_t sample = delivered ? 0u : 100u;

    /* EMA на packet_loss, вага нового зразка 1/8 */
    n->packet_loss = (uint8_t)(((uint32_t)n->packet_loss * 7u + sample) / 8u);
}

void rp_neighbor_expire(rp_neighbor_table_t *t, uint32_t now_ms, uint32_t timeout_ms)
{
    if (!t) return;

    for (uint16_t i = 0; i < RP_P2P_MAX_NEIGHBORS; i++)
    {
        rp_neighbor_t *n = &t->entries[i];

        if (n->in_use && ((now_ms - n->last_seen_ms) >= timeout_ms))
        {
            memset(n, 0, sizeof(*n));
        }
    }
}

static uint8_t clamp_u8(int v)
{
    if (v < 0) return 0u;
    if (v > 100) return 100u;
    return (uint8_t)v;
}

static uint8_t rssi_score(int16_t rssi_dbm)
{
    /* -100dBm -> 0, -40dBm -> 100, лінійно */
    int v = ((int)rssi_dbm + 100) * 100 / 60;
    return clamp_u8(v);
}

static uint8_t snr_score(int16_t snr_db)
{
    /* 0dB -> 0, 30dB -> 100 */
    int v = (int)snr_db * 100 / 30;
    return clamp_u8(v);
}

static uint8_t latency_score(uint16_t latency_ms)
{
    /* 0ms -> 100, 500ms+ -> 0, лінійно */
    if (latency_ms >= 500u) return 0u;
    return clamp_u8(100 - ((int)latency_ms * 100 / 500));
}

uint8_t rp_neighbor_quality(const rp_neighbor_t *n)
{
    if (!n) return 0u;

    uint8_t rs = rssi_score(n->rssi);
    uint8_t ss = snr_score(n->snr);
    uint8_t ls = latency_score(n->latency_ms);
    uint8_t loss_score = clamp_u8(100 - (int)n->packet_loss);

    /* Ваги суб'єктивні, але навмисно НЕ 100% на rssi (ТЗ §12) —
     * packet_loss важить найбільше (пряме свідчення надійності), rssi
     * другий, snr і затримка — уточнюючі. */
    uint32_t weighted = (uint32_t)rs * 30u + (uint32_t)ss * 15u
                       + (uint32_t)loss_score * 35u + (uint32_t)ls * 20u;

    return (uint8_t)(weighted / 100u);
}
