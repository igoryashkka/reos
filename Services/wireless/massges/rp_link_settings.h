#ifndef RP_LINK_SETTINGS_H
#define RP_LINK_SETTINGS_H

#include "rp_link.h"
#include "rp_link_internal.h"
#include "rp_msg.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int rp_link_build_settings(rp_link_t *link, uint8_t *out, size_t out_cap,
                                         const rp_param_t *params, uint8_t count, bool write)
{
    rp_msg_t msg;

    if ((link == NULL) || (out == NULL) || ((params == NULL) && (count != 0u)))
    {
        return RP_MSG_E_ARG;
    }
    if (count > RP_MAX_PARAMS)
    {
        return RP_MSG_E_TOO_MANY;
    }

    memset(&msg, 0, sizeof(msg));
    msg.type = RP_T_CFG_APP;
    msg.u.cfg.op = write ? RP_CFG_SET : RP_CFG_GET;
    msg.u.cfg.n = count;
    for (uint8_t index = 0; index < count; index++)
    {
        msg.u.cfg.p[index] = params[index];
    }

    return rp_link_encode(link, &msg, out, out_cap);
}

#ifdef __cplusplus
}
#endif

#endif /* RP_LINK_SETTINGS_H */