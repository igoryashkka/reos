#ifndef RP_LINK_PING_H
#define RP_LINK_PING_H

#include "rp_link.h"
#include "rp_link_internal.h"
#include "rp_msg.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int rp_link_build_ping(rp_link_t *link, uint8_t *out, size_t out_cap, uint32_t nonce)
{
	rp_msg_t msg;

	if ((link == NULL) || (out == NULL))
	{
		return RP_MSG_E_ARG;
	}

	memset(&msg, 0, sizeof(msg));
	msg.type = RP_T_PING;
	msg.u.ping.nonce = nonce;

	return rp_link_encode(link, &msg, out, out_cap);
}

#ifdef __cplusplus
}
#endif

#endif /* RP_LINK_PING_H */