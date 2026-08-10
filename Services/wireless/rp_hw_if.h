#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rp_hw_if
{
    const char *name;

    int (*init)(void);

    int (*send)(const uint8_t *data, size_t length);

} rp_hw_if_t;

typedef enum
{
    RP_HW_IF_UART = 0,
    RP_HW_IF_RADIO,

    RP_HW_IF_COUNT
} rp_hw_if_id_t;

void rp_hw_if_init_all(void);

int rp_hw_if_select(rp_hw_if_id_t id);

int rp_hw_if_send(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif
