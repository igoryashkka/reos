#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Облік частки часу з увімкненим радіо (TX/RX), усереднений по вікну, що
 * котиться. rp_duty_percent() читає стан лендінгу — можна викликати будь-коли
 * для логів/DEV_INFO, не лише на межі вікна. */
typedef struct {
    uint32_t window_start_ms;
    uint32_t window_len_ms;
    uint32_t on_accum_ms;
    uint32_t on_since_ms;   /* дійсне лише коли is_on */
    uint8_t  is_on;
    uint8_t  last_pct;
} rp_duty_t;

void    rp_duty_init(rp_duty_t *d, uint32_t now_ms, uint32_t window_len_ms);
void    rp_duty_on(rp_duty_t *d, uint32_t now_ms);
void    rp_duty_off(rp_duty_t *d, uint32_t now_ms);
uint8_t rp_duty_percent(rp_duty_t *d, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
