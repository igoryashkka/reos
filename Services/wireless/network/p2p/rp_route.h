#pragma once

#include "rp_neighbor.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RP_P2P_MAX_ROUTES
#define RP_P2P_MAX_ROUTES 16u
#endif

typedef struct {
    uint8_t  in_use;
    uint16_t destination;
    uint16_t next_hop;
    uint16_t metric;    /* нижче = краще */
    uint32_t expires_ms;
} rp_route_t;

typedef struct {
    rp_route_t entries[RP_P2P_MAX_ROUTES];
} rp_route_table_t;

void rp_route_init(rp_route_table_t *t);

/* NULL, якщо запису немає АБО він прострочений на now_ms */
rp_route_t *rp_route_find(rp_route_table_t *t, uint16_t destination, uint32_t now_ms);

/* Вставляє новий запис або замінює/освіжає існуючий для того самого
 * destination: кращий (нижчий) metric чи оновлення від того самого
 * next_hop перемагає, гірший candidate від ІНШОГО next_hop ігнорується
 * (route replacement, ТЗ §10-11). */
rp_route_t *rp_route_upsert(
    rp_route_table_t *t,
    uint16_t destination,
    uint16_t next_hop,
    uint16_t metric,
    uint32_t now_ms,
    uint32_t ttl_ms
);

void rp_route_expire(rp_route_table_t *t, uint32_t now_ms);
void rp_route_remove(rp_route_table_t *t, rp_route_t *r);

/* Policy function (ТЗ §12): формула метрики винесена окремо, а не
 * прив'язана напряму до RSSI. hop_count — кількість хопів через
 * via_neighbor включно. */
uint16_t rp_route_metric_default(const rp_neighbor_t *via_neighbor, uint16_t hop_count);

#ifdef __cplusplus
}
#endif
