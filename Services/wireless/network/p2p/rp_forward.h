#pragma once

#include "rp_proto.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RP_P2P_DUP_CACHE_SIZE
#define RP_P2P_DUP_CACHE_SIZE 16u
#endif

#ifndef RP_P2P_DUP_WINDOW_MS
#define RP_P2P_DUP_WINDOW_MS 30000u
#endif

/* Ідентичність пакета для дедупу — (src, seq), а не (dst, seq): src/seq
 * не змінюються по дорозі (rp_forward_rebuild лише зменшує TTL/frag),
 * тож пара лишається стабільною на всьому шляху пакета через mesh. */
typedef struct {
    uint8_t  in_use;
    uint16_t src;
    uint8_t  seq;
    uint32_t seen_ms;
} rp_dup_entry_t;

typedef struct {
    rp_dup_entry_t entries[RP_P2P_DUP_CACHE_SIZE];
    uint16_t       next_evict;
} rp_dup_cache_t;

void rp_dup_cache_init(rp_dup_cache_t *c);

/* 1, якщо (src,seq) вже бачили в межах RP_P2P_DUP_WINDOW_MS (дублікат,
 * кеш не займає новий слот); 0, якщо новий — і одразу заносить його. */
int rp_dup_cache_check_and_add(rp_dup_cache_t *c, uint16_t src, uint8_t seq, uint32_t now_ms);

/* Перебудовує прийнятий кадр для ретрансляції зі зменшеним TTL (байт
 * frag — домовленість ТЗ: власного поля TTL у заголовку немає, frag
 * поза bulk/OTA (не в межах цього ТЗ) інакше не використовується).
 * -1, якщо TTL уже вичерпано (frag == 0) — форвардити не можна.
 * out_ttl_after (опційно) — TTL, що пішов у зібраний кадр. */
int rp_forward_rebuild(uint8_t *out, size_t out_cap, const rp_frame_t *f, int8_t *out_ttl_after);

#ifdef __cplusplus
}
#endif
