#ifndef RP_LINK_AUTH_H
#define RP_LINK_AUTH_H

#include "rp_link.h"
#include "rp_link_internal.h"
#include "rp_msg.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int rp_link_build_auth(rp_link_t *link, uint8_t *out, size_t out_cap,
                                     const uint8_t dev_id[RP_DEV_ID_LEN], uint32_t response)
{
    rp_msg_t msg;

    if ((link == NULL) || (out == NULL) || (dev_id == NULL))
    {
        return RP_MSG_E_ARG;
    }

    memset(&msg, 0, sizeof(msg));
    msg.type = RP_T_AUTH;
    memcpy(msg.u.auth.dev_id, dev_id, RP_DEV_ID_LEN);
    msg.u.auth.response = response;

    return rp_link_encode(link, &msg, out, out_cap);
}

#ifdef __cplusplus
}
#endif

#endif /* RP_LINK_AUTH_H */