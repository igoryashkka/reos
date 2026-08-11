#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RP_P2P_MAX_NEIGHBORS
#define RP_P2P_MAX_NEIGHBORS 16u
#endif

typedef struct {
    uint8_t  in_use;
    uint16_t addr;
    int16_t  rssi;
    int16_t  snr;
    uint16_t latency_ms;
    uint8_t  packet_loss;   /* 0..100 %, EMA */
    uint32_t last_seen_ms;
} rp_neighbor_t;

typedef struct {
    rp_neighbor_t entries[RP_P2P_MAX_NEIGHBORS];
} rp_neighbor_table_t;

void rp_neighbor_init(rp_neighbor_table_t *t);

rp_neighbor_t *rp_neighbor_find(rp_neighbor_table_t *t, uint16_t addr);

/* Знаходить існуючий запис за addr, інакше займає вільний слот, інакше
 * витісняє найдавніше бачений (LRU). Скидає статистику при вставці
 * нового/витісненого сусіда. */
rp_neighbor_t *rp_neighbor_touch(rp_neighbor_table_t *t, uint16_t addr, uint32_t now_ms, int16_t rssi, int16_t snr);

void rp_neighbor_note_latency(rp_neighbor_t *n, uint16_t latency_ms);
void rp_neighbor_note_delivery(rp_neighbor_t *n, int delivered);

void rp_neighbor_expire(rp_neighbor_table_t *t, uint32_t now_ms, uint32_t timeout_ms);

/* 0..100, вище — краще. Комбінує rssi/snr/packet_loss/latency — НЕ
 * прив'язано лише до RSSI (ТЗ §12). */
uint8_t rp_neighbor_quality(const rp_neighbor_t *n);

#ifdef __cplusplus
}
#endif
