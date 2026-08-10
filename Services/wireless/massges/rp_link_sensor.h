#ifndef RP_LINK_SENSOR_H
#define RP_LINK_SENSOR_H

#include "rp_link.h"
#include "rp_link_internal.h"
#include "rp_msg.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int rp_link_build_sensor(rp_link_t *link, uint8_t *out, size_t out_cap,
                                       const rp_reading_t *readings, uint8_t count)
{
    rp_msg_t msg;

    if ((link == NULL) || (out == NULL) || (readings == NULL) || (count == 0u))
    {
        return RP_MSG_E_ARG;
    }
    if (count > RP_MAX_READINGS)
    {
        return RP_MSG_E_TOO_MANY;
    }

    memset(&msg, 0, sizeof(msg));
    msg.type = RP_T_SENSOR;
    msg.u.sensor.batch_tag = link->tag++;
    msg.u.sensor.n = count;
    for (uint8_t index = 0; index < count; index++)
    {
        msg.u.sensor.r[index] = readings[index];
    }

    return rp_link_send(link, &msg, out, out_cap);
}

#ifdef __cplusplus
}
#endif

#endif /* RP_LINK_SENSOR_H */