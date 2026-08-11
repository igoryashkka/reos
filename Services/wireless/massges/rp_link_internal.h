#ifndef RP_LINK_INTERNAL_H
#define RP_LINK_INTERNAL_H

#include "rp_link.h"
#include "rp_msg.h"

/* Лише кодує в out — попри стару назву "send", в ефір нічого не йде.
 * Фактичну передачу виконує викликач через rp_hw_if_send() (App/ або
 * roles/), інакше кадр був би переданий двічі. */
static inline int rp_link_encode(rp_link_t *link, const rp_msg_t *msg, uint8_t *out, size_t out_cap)
{
	int rc;

	if ((link == NULL) || (msg == NULL) || (out == NULL))
	{
		return RP_MSG_E_ARG;
	}

	rc = rp_msg_build(out, out_cap, msg, link->addr, link->gateway, link->seq, 0u);
	if (rc >= 0)
	{
		link->seq++;
	}

	return rc;
}

#endif /* RP_LINK_INTERNAL_H */