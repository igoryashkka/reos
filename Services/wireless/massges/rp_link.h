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

#define RP_LINK_TX_CAP RP_FRAME_MAX

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

#ifdef __cplusplus
}
#endif

#endif /* RP_LINK_H */