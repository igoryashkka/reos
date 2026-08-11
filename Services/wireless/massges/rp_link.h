#ifndef RP_LINK_H
#define RP_LINK_H

#include "rp_proto.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t addr;
    uint16_t gateway;
    uint8_t  seq;
    uint16_t tag;
} rp_link_t;

/* RP_FRAME_MAX (rp_proto.h) не враховує RP_PREAMBLE_LEN, який rp_build()
 * усе одно пише в ефір — тому тут явно RP_OVERHEAD (включає преамбулу)
 * + RP_MAX_PAYLOAD, а не голий RP_FRAME_MAX. */
#define RP_LINK_TX_CAP (RP_OVERHEAD + RP_MAX_PAYLOAD)

static inline void rp_link_init(rp_link_t *link)
{
    if (link == NULL)
    {
        return;
    }

    link->addr = RP_ADDR_UNASSIGNED;
    link->gateway = RP_ADDR_BROADCAST;
    link->seq = 0u;
    link->tag = 1u;
}

/* Єдине місце виділення tag: 0 зарезервовано як "без тегу", тому
 * лічильник пропускає його при переповненні. Білдери (rp_link_build_*),
 * яким потрібен свіжий tag для власного повідомлення, звертаються сюди
 * замість того, щоб інкрементувати link->tag напряму. */
static inline uint16_t rp_link_alloc_tag(rp_link_t *link)
{
    uint16_t tag = link->tag++;

    if (link->tag == 0u)
    {
        link->tag = 1u;
    }

    return tag;
}

#ifdef __cplusplus
}
#endif

#endif /* RP_LINK_H */