/* ==================================================================
 * rp_iot_router.c — явна заглушка (ТЗ §3.3, підтверджено новим ТЗ §17).
 *
 * Ретрансляція (leaf-телеметрія власного вузла + вузький підмножини
 * gateway-логіки лише для прямих дітей) у межах цього спринту не
 * реалізована. Спроба використання гучно падає — LOG_ERROR + пастка,
 * а не мовчазний no-op, як прямо вимагає ТЗ. Не смикає Core/Inc/main.h
 * (Error_Handler) навмисно: цей файл мусить лінкуватись і на хості
 * (tests/wireless/, RP_NETWORK_MODEL=host_test), де платформного
 * main.h немає.
 * ================================================================== */
#include "rp_network.h"

#include "logger.h"

static void router_not_implemented(const char *fn)
{
    LOG_ERROR(LOG_MODULE_APP, "rp_iot_router: %s not implemented (TZ stub)", fn);

    for (;;)
    {
        /* навмисна пастка: краще зависнути гучно, ніж мовчки продовжити
         * без реальної маршрутизації */
    }
}

void rp_network_init(const rp_network_cfg_t *cfg)
{
    (void)cfg;
    router_not_implemented("rp_network_init");
}

int rp_network_submit(const rp_msg_t *msg, rp_network_prio_t prio)
{
    (void)msg;
    (void)prio;
    router_not_implemented("rp_network_submit");
    return RP_MSG_E_UNKNOWN;
}

void rp_network_on_frame(const rp_frame_t *f, uint32_t now_ms)
{
    (void)f;
    (void)now_ms;
    router_not_implemented("rp_network_on_frame");
}

void rp_network_tick(uint32_t now_ms)
{
    (void)now_ms;
    router_not_implemented("rp_network_tick");
}

uint32_t rp_network_next_deadline_ms(uint32_t now_ms)
{
    (void)now_ms;
    router_not_implemented("rp_network_next_deadline_ms");
    return 0u;
}
