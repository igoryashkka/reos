/* ==================================================================
 * rp_iot_leaf.c — сенсорний вузол: сон більшість часу, TX за
 * розкладом, рівно один inflight-слот на ретрай.
 *
 * Час — виключно через now_ms-параметри контракту (rp_network.h); прямі
 * платформні виклики (xTaskGetTickCount/HAL_GetTick) тут заборонені.
 * ================================================================== */
#include "rp_network.h"
#include "rp_iot_leaf.h"

#include "rp_duty.h"
#include "rp_frameq.h"
#include "rp_hw_if.h"
#include "rp_link.h"
#include "rp_link_auth.h"
#include "rp_link_ping.h"
#include "rp_link_register.h"
#include "rp_link_sensor.h"
#include "rp_msg.h"

#include <string.h>

#ifndef RP_LEAF_QUEUE_DEPTH
#define RP_LEAF_QUEUE_DEPTH 16u
#endif

#ifndef RP_LEAF_TX_INTERVAL_MS
#define RP_LEAF_TX_INTERVAL_MS 30000u
#endif

#ifndef RP_LEAF_PING_INTERVAL_MS
#define RP_LEAF_PING_INTERVAL_MS 60000u
#endif

#ifndef RP_LEAF_ACK_TIMEOUT_MS
#define RP_LEAF_ACK_TIMEOUT_MS 2000u
#endif

#ifndef RP_LEAF_MAX_RETRIES
#define RP_LEAF_MAX_RETRIES 3u
#endif

#ifndef RP_IOT_RX_WINDOW_MS
#define RP_IOT_RX_WINDOW_MS 500u
#endif

#ifndef RP_LEAF_DUTY_WINDOW_MS
#define RP_LEAF_DUTY_WINDOW_MS 600000u /* 10 хв */
#endif

typedef enum {
    LEAF_STAGE_REGISTER = 0,
    LEAF_STAGE_AUTH,
    LEAF_STAGE_RUN
} leaf_stage_t;

typedef struct {
    rp_reading_t items[RP_LEAF_QUEUE_DEPTH];
    uint16_t     head;
    uint16_t     count;
    uint32_t     drop_count;
} leaf_reading_q_t;

typedef struct {
    uint8_t  active;
    uint8_t  buf[RP_LINK_TX_CAP];
    uint16_t len;
    uint8_t  type;      /* rp_type_t того, що надіслано — з чим звіряти відповідь */
    uint8_t  retries;
    uint32_t deadline_ms;
} leaf_inflight_t;

typedef struct {
    rp_network_cfg_t     cfg;
    rp_link_t         link;
    leaf_stage_t      stage;
    uint8_t           time_ready;

    leaf_reading_q_t  rq;

    uint8_t           urgent_pending;
    rp_msg_t          urgent_msg;

    leaf_inflight_t   inflight;

    rp_frameq_t       rxq;             /* глибина 1-2: коротке rx-вікно */
    uint8_t           rx_window_open;
    uint32_t          rx_window_deadline_ms;

    uint32_t          next_tx_ms;
    uint32_t          next_ping_ms;

    rp_duty_t         duty;
} leaf_t;

static leaf_t g_leaf;

static uint32_t leaf_min_ms(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

static void leaf_reading_push(const rp_reading_t *r)
{
    leaf_reading_q_t *q = &g_leaf.rq;

    if (q->count == RP_LEAF_QUEUE_DEPTH)
    {
        q->head = (uint16_t)((q->head + 1u) % RP_LEAF_QUEUE_DEPTH);
        q->count--;
        q->drop_count++;
    }

    q->items[(uint16_t)((q->head + q->count) % RP_LEAF_QUEUE_DEPTH)] = *r;
    q->count++;
}

static void leaf_schedule_init(uint32_t now_ms)
{
    g_leaf.next_tx_ms   = now_ms + RP_LEAF_TX_INTERVAL_MS;
    g_leaf.next_ping_ms = now_ms + RP_LEAF_PING_INTERVAL_MS;
    rp_duty_init(&g_leaf.duty, now_ms, RP_LEAF_DUTY_WINDOW_MS);
    g_leaf.time_ready = 1u;
}

static void leaf_close_inflight(uint32_t now_ms)
{
    g_leaf.inflight.active = 0u;

    if (g_leaf.rx_window_open)
    {
        g_leaf.rx_window_open = 0u;
        rp_duty_off(&g_leaf.duty, now_ms);
    }
}

/* (Пере)передає поточний inflight.buf, відкриває rx-вікно й ack-таймаут.
 * Усі повідомлення, які надсилає leaf, ідуть з RP_F_ACK_REQ (rp_msg.c
 * k_desc[]), тож вікно відкривається завжди. */
static void leaf_transmit_inflight(uint32_t now_ms)
{
    rp_duty_on(&g_leaf.duty, now_ms);
    rp_hw_if_send(g_leaf.inflight.buf, (size_t)g_leaf.inflight.len);

    g_leaf.rx_window_open        = 1u;
    g_leaf.rx_window_deadline_ms = now_ms + RP_IOT_RX_WINDOW_MS;
    g_leaf.inflight.deadline_ms   = now_ms + RP_LEAF_ACK_TIMEOUT_MS;
}

static void leaf_start_inflight(uint8_t type, const uint8_t *buf, uint16_t len, uint32_t now_ms)
{
    memcpy(g_leaf.inflight.buf, buf, len);
    g_leaf.inflight.len     = len;
    g_leaf.inflight.type    = type;
    g_leaf.inflight.retries = 0u;
    g_leaf.inflight.active  = 1u;

    leaf_transmit_inflight(now_ms);
}

static void leaf_send_register(uint32_t now_ms)
{
    uint8_t buf[RP_LINK_TX_CAP];
    int len = rp_link_build_register(
        &g_leaf.link, buf, sizeof(buf),
        g_leaf.cfg.dev_id, g_leaf.cfg.hw_ver, g_leaf.cfg.fw_ver, g_leaf.cfg.caps
    );

    if (len < 0) return;

    leaf_start_inflight(RP_T_REGISTER, buf, (uint16_t)len, now_ms);
}

static void leaf_send_auth(uint32_t now_ms)
{
    uint8_t buf[RP_LINK_TX_CAP];
    /* челендж/відповідь — окреме ТЗ (§7); поки що заглушка на now_ms,
     * як і в App/application.c до появи цього шару. */
    int len = rp_link_build_auth(&g_leaf.link, buf, sizeof(buf), g_leaf.cfg.dev_id, now_ms);

    if (len < 0) return;

    leaf_start_inflight(RP_T_AUTH, buf, (uint16_t)len, now_ms);
}

static void leaf_send_sensor_batch(uint32_t now_ms)
{
    rp_reading_t batch[RP_MAX_READINGS];
    uint8_t n = 0u;

    while ((n < RP_MAX_READINGS) && (g_leaf.rq.count > 0u))
    {
        batch[n] = g_leaf.rq.items[g_leaf.rq.head];
        g_leaf.rq.head = (uint16_t)((g_leaf.rq.head + 1u) % RP_LEAF_QUEUE_DEPTH);
        g_leaf.rq.count--;
        n++;
    }

    uint8_t buf[RP_LINK_TX_CAP];
    int len = rp_link_build_sensor(&g_leaf.link, buf, sizeof(buf), batch, n);

    g_leaf.next_tx_ms = now_ms + RP_LEAF_TX_INTERVAL_MS;

    if (len < 0) return;

    leaf_start_inflight(RP_T_SENSOR, buf, (uint16_t)len, now_ms);
}

static void leaf_send_ping(uint32_t now_ms)
{
    uint8_t buf[RP_LINK_TX_CAP];
    int len = rp_link_build_ping(&g_leaf.link, buf, sizeof(buf), now_ms);

    g_leaf.next_ping_ms = now_ms + RP_LEAF_PING_INTERVAL_MS;

    if (len < 0) return;

    leaf_start_inflight(RP_T_PING, buf, (uint16_t)len, now_ms);
}

/* RP_T_ALARM не має власного rp_link_build_*; будуємо напряму через
 * rp_msg_build (те саме, що робить rp_link_send всередині), щоб не
 * чіпати massges/rp_link*.h поза дозволеним списком (ТЗ §4). */
static void leaf_send_urgent(uint32_t now_ms)
{
    uint8_t buf[RP_LINK_TX_CAP];
    int len = rp_msg_build(
        buf, sizeof(buf), &g_leaf.urgent_msg,
        g_leaf.link.addr, g_leaf.link.gateway, g_leaf.link.seq, 0u
    );

    if (len < 0) return;

    g_leaf.link.seq++;
    g_leaf.urgent_pending = 0u;

    leaf_start_inflight(RP_T_ALARM, buf, (uint16_t)len, now_ms);
}

static void leaf_handle_frame(const rp_frame_t *f, uint32_t now_ms)
{
    rp_msg_t decoded;

    if (rp_msg_decode(f, &decoded) != RP_MSG_OK) return;

    switch (decoded.type)
    {
    case RP_T_REG_RESP:
        if (g_leaf.inflight.active && (g_leaf.inflight.type == RP_T_REGISTER))
        {
            leaf_close_inflight(now_ms);

            if (decoded.u.reg_resp.status == RP_OK)
            {
                g_leaf.link.addr    = decoded.u.reg_resp.short_addr;
                g_leaf.link.gateway = f->hdr.src;
                g_leaf.stage         = LEAF_STAGE_AUTH;
            }
        }
        break;

    case RP_T_AUTH_RESP:
        if (g_leaf.inflight.active && (g_leaf.inflight.type == RP_T_AUTH))
        {
            leaf_close_inflight(now_ms);
            g_leaf.stage = (decoded.u.auth_resp.status == RP_OK) ? LEAF_STAGE_RUN : LEAF_STAGE_REGISTER;
        }
        break;

    case RP_T_PONG:
        if (g_leaf.inflight.active && (g_leaf.inflight.type == RP_T_PING))
        {
            leaf_close_inflight(now_ms);
        }
        break;

    case RP_T_ACK:
        if (g_leaf.inflight.active && (decoded.u.ack.acked_type == g_leaf.inflight.type))
        {
            leaf_close_inflight(now_ms);
        }
        break;

    default:
        break; /* NACK і решта — просто дочекаємось ack-таймауту й зробимо ретрай */
    }
}

static void leaf_drain_rxq(uint32_t now_ms)
{
    rp_frameq_item_t item;

    while (rp_frameq_pop(&g_leaf.rxq, &item))
    {
        rp_frame_t f;

        f.hdr      = item.hdr;
        f.core     = item.core;
        f.tail     = item.tail;
        f.tail_len = item.tail_len;
        f.rssi     = item.rssi;
        f.snr      = item.snr;

        leaf_handle_frame(&f, now_ms);
    }
}

void rp_network_init(const rp_network_cfg_t *cfg)
{
    memset(&g_leaf, 0, sizeof(g_leaf));

    if (cfg)
    {
        g_leaf.cfg = *cfg;
    }

    rp_link_init(&g_leaf.link);
    g_leaf.link.addr    = g_leaf.cfg.addr;
    g_leaf.link.gateway = g_leaf.cfg.gateway_addr;

    g_leaf.stage = LEAF_STAGE_REGISTER;
    rp_frameq_init(&g_leaf.rxq, 2u);

    rp_hw_if_select(g_leaf.cfg.hw_if);
}

int rp_network_submit(const rp_msg_t *msg, rp_network_prio_t prio)
{
    if (!msg) return RP_MSG_E_ARG;

    if (prio == RP_NETWORK_PRIO_URGENT)
    {
        if (msg->type != RP_T_ALARM) return RP_MSG_E_ARG;

        g_leaf.urgent_msg     = *msg;
        g_leaf.urgent_pending = 1u;

        return RP_MSG_OK;
    }

    if (msg->type != RP_T_SENSOR) return RP_MSG_E_UNKNOWN;

    for (uint8_t i = 0; i < msg->u.sensor.n; i++)
    {
        leaf_reading_push(&msg->u.sensor.r[i]);
    }

    return RP_MSG_OK;
}

void rp_network_on_frame(const rp_frame_t *f, uint32_t now_ms)
{
    if (!f) return;

    if (rp_frameq_push(&g_leaf.rxq, f) != 0)
    {
        return; /* rx-вікно переповнене (глибина 1-2) — кадр відкинуто */
    }

    leaf_drain_rxq(now_ms); /* обробка синхронна: спорожняємо одразу ж */
}

void rp_network_tick(uint32_t now_ms)
{
    if (!g_leaf.time_ready)
    {
        leaf_schedule_init(now_ms);
    }

    if (g_leaf.rx_window_open && (now_ms >= g_leaf.rx_window_deadline_ms))
    {
        g_leaf.rx_window_open = 0u;
        rp_duty_off(&g_leaf.duty, now_ms);
    }

    if (g_leaf.inflight.active)
    {
        if (now_ms >= g_leaf.inflight.deadline_ms)
        {
            if (g_leaf.inflight.retries < RP_LEAF_MAX_RETRIES)
            {
                g_leaf.inflight.retries++;
                leaf_transmit_inflight(now_ms);
            }
            else
            {
                leaf_close_inflight(now_ms);
            }
        }

        return; /* рівно один inflight-слот: нічого нового, доки він зайнятий */
    }

    if (g_leaf.stage == LEAF_STAGE_REGISTER)
    {
        leaf_send_register(now_ms);
        return;
    }

    if (g_leaf.stage == LEAF_STAGE_AUTH)
    {
        leaf_send_auth(now_ms);
        return;
    }

    if (g_leaf.urgent_pending)
    {
        leaf_send_urgent(now_ms);
        return;
    }

    if ((now_ms >= g_leaf.next_tx_ms) && (g_leaf.rq.count > 0u))
    {
        leaf_send_sensor_batch(now_ms);
        return;
    }

    if (now_ms >= g_leaf.next_ping_ms)
    {
        leaf_send_ping(now_ms);
        return;
    }
}

uint32_t rp_network_next_deadline_ms(uint32_t now_ms)
{
    if (!g_leaf.time_ready) return 0u;

    uint8_t  have_deadline = 0u;
    uint32_t deadline = 0u;

    if (g_leaf.inflight.active)
    {
        deadline = g_leaf.inflight.deadline_ms;
        have_deadline = 1u;

        if (g_leaf.rx_window_open)
        {
            deadline = leaf_min_ms(deadline, g_leaf.rx_window_deadline_ms);
        }
    }
    else if (g_leaf.urgent_pending || (g_leaf.stage != LEAF_STAGE_RUN))
    {
        return 0u; /* негайно є робота */
    }
    else
    {
        deadline = g_leaf.next_ping_ms;
        have_deadline = 1u;

        if (g_leaf.rq.count > 0u)
        {
            deadline = leaf_min_ms(deadline, g_leaf.next_tx_ms);
        }
    }

    if (!have_deadline) return 0u;

    return (deadline > now_ms) ? (deadline - now_ms) : 0u;
}

uint32_t rp_iot_leaf_reading_drops(void)
{
    return g_leaf.rq.drop_count;
}

uint8_t rp_iot_leaf_duty_percent(uint32_t now_ms)
{
    return rp_duty_percent(&g_leaf.duty, now_ms);
}
