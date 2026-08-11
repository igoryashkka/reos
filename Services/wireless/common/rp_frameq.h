#pragma once

#include "rp_proto.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Одна пріоритетна черга = один rp_frameq_t. "Дві черги з пріоритетами"
 * (roles/rp_role_gateway.c: urgent + bulk) — це два незалежні інстанси,
 * не одна структура з двома масивами всередині. */

#ifndef RP_FRAMEQ_MAX_DEPTH
#define RP_FRAMEQ_MAX_DEPTH 8u
#endif

/* rp_frame_t.tail показує в буфер парсера і живе лише до наступного
 * rp_parse_byte(); тут кадр копіюється цілком, щоб пережити чергу. */
typedef struct {
    rp_hdr_t  hdr;
    rp_core_t core;
    uint8_t   tail[RP_MAX_TAIL];
    uint16_t  tail_len;
    int8_t    rssi;
    int8_t    snr;
} rp_frameq_item_t;

typedef struct {
    rp_frameq_item_t items[RP_FRAMEQ_MAX_DEPTH];
    uint16_t          capacity;
    uint16_t          head;
    uint16_t          count;
} rp_frameq_t;

/* capacity понад RP_FRAMEQ_MAX_DEPTH обрізається до нього */
void rp_frameq_init(rp_frameq_t *q, uint16_t capacity);

/* копіює f->tail у власний буфер елемента; -1 якщо черга повна або tail_len задовгий */
int  rp_frameq_push(rp_frameq_t *q, const rp_frame_t *f);

/* 1 якщо взяли елемент, 0 якщо черга порожня */
int  rp_frameq_pop(rp_frameq_t *q, rp_frameq_item_t *out);

int  rp_frameq_is_empty(const rp_frameq_t *q);
int  rp_frameq_is_full(const rp_frameq_t *q);

#ifdef __cplusplus
}
#endif
