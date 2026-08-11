#include "rp_mailbox.h"

#include <string.h>

void rp_mailbox_init(rp_mailbox_t *mb)
{
    if (!mb) return;

    memset(mb->slots, 0, sizeof(mb->slots));
}

int rp_mailbox_put(rp_mailbox_t *mb, uint16_t index, const rp_msg_t *msg)
{
    if (!mb || !msg || (index >= RP_MAX_NODES)) return -1;

    mb->slots[index].msg     = *msg;
    mb->slots[index].pending = 1u;

    return 0;
}

int rp_mailbox_has_pending(const rp_mailbox_t *mb, uint16_t index)
{
    if (!mb || (index >= RP_MAX_NODES)) return 0;

    return mb->slots[index].pending;
}

int rp_mailbox_take(rp_mailbox_t *mb, uint16_t index, rp_msg_t *out_msg)
{
    if (!mb || !out_msg || (index >= RP_MAX_NODES)) return 0;

    if (!mb->slots[index].pending) return 0;

    *out_msg = mb->slots[index].msg;
    mb->slots[index].pending = 0u;

    return 1;
}

void rp_mailbox_clear(rp_mailbox_t *mb, uint16_t index)
{
    if (!mb || (index >= RP_MAX_NODES)) return;

    mb->slots[index].pending = 0u;
}
