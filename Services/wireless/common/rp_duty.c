#include "rp_duty.h"

void rp_duty_init(rp_duty_t *d, uint32_t now_ms, uint32_t window_len_ms)
{
    if (!d) return;

    d->window_start_ms = now_ms;
    d->window_len_ms    = window_len_ms;
    d->on_accum_ms      = 0u;
    d->on_since_ms       = 0u;
    d->is_on              = 0u;
    d->last_pct          = 0u;
}

void rp_duty_on(rp_duty_t *d, uint32_t now_ms)
{
    if (!d) return;
    if (d->is_on) return;

    d->is_on       = 1u;
    d->on_since_ms = now_ms;
}

void rp_duty_off(rp_duty_t *d, uint32_t now_ms)
{
    if (!d) return;
    if (!d->is_on) return;

    d->on_accum_ms += (now_ms - d->on_since_ms);
    d->is_on = 0u;
}

uint8_t rp_duty_percent(rp_duty_t *d, uint32_t now_ms)
{
    if (!d) return 0u;

    uint32_t elapsed = now_ms - d->window_start_ms;

    if (elapsed == 0u)
    {
        return d->last_pct;
    }

    uint32_t on_ms = d->on_accum_ms;

    if (d->is_on)
    {
        on_ms += (now_ms - d->on_since_ms);
    }

    uint8_t pct = (uint8_t)((on_ms * 100u) / elapsed);

    if (elapsed >= d->window_len_ms)
    {
        d->window_start_ms = now_ms;
        d->on_accum_ms      = 0u;

        if (d->is_on)
        {
            d->on_since_ms = now_ms;
        }
    }

    d->last_pct = pct;

    return pct;
}
