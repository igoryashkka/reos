/* ==================================================================
 * rp_iot_gateway.c — мережеве живлення: приймає й агрегує потоки від
 * багатьох leaf. now_ms — виключно з параметрів контракту (rp_network.h).
 *
 * Вікно доставки mailbox: спец не визначає окремий протокол
 * узгодження часу пробудження вузла (next_wake_ms лишається полем на
 * майбутнє, discovery/scheduling — окреме ТЗ, §7). Тут доставка
 * опортуністична: pending downlink іде відразу слідом за відповіддю
 * на будь-який вхідний кадр від цього вузла — це єдиний момент, коли
 * приймач вузла гарантовано увімкнений.
 * ================================================================== */
#include "rp_network.h"
#include "rp_iot_gateway.h"

#include "rp_frameq.h"
#include "rp_hw_if.h"
#include "rp_mailbox.h"
#include "rp_msg.h"
#include "rp_nodetab.h"

#include <string.h>

#ifndef RP_IOT_KEEPALIVE_MS
#define RP_IOT_KEEPALIVE_MS 120000u
#endif

/* без окремого перевизначення падають на спільний RP_FRAMEQ_DEPTH
 * (common/, завжди визначений — rp_configure_network() задає його для
 * будь-якої моделі/ролі) */
#ifndef RP_GW_URGENT_QUEUE_DEPTH
#define RP_GW_URGENT_QUEUE_DEPTH RP_FRAMEQ_DEPTH
#endif

#ifndef RP_GW_BULK_QUEUE_DEPTH
#define RP_GW_BULK_QUEUE_DEPTH RP_FRAMEQ_DEPTH
#endif

#ifndef RP_GW_IDLE_POLL_MS
#define RP_GW_IDLE_POLL_MS 5000u
#endif

typedef struct {
    rp_network_cfg_t cfg;
    rp_nodetab_t  nodetab;
    rp_mailbox_t  mailbox;
    rp_frameq_t   urgent_q;
    rp_frameq_t   bulk_q;
    uint16_t      next_short_addr;
    uint8_t       tx_seq;
} gw_t;

static gw_t g_gw;

static uint16_t gw_node_index(const rp_node_entry_t *e)
{
    return (uint16_t)(e - g_gw.nodetab.entries);
}

static uint16_t gw_alloc_addr(void)
{
    uint16_t addr;

    do
    {
        g_gw.next_short_addr++;
        if ((g_gw.next_short_addr == RP_ADDR_UNASSIGNED) || (g_gw.next_short_addr == RP_ADDR_BROADCAST))
        {
            g_gw.next_short_addr = 1u;
        }
        addr = g_gw.next_short_addr;
    } while ((addr == g_gw.cfg.addr) || (rp_nodetab_find_by_addr(&g_gw.nodetab, addr) != NULL));

    return addr;
}

static void gw_send_msg(const rp_msg_t *msg, uint16_t dst)
{
    uint8_t buf[RP_FRAME_MAX];
    int len = rp_msg_build(buf, sizeof(buf), msg, g_gw.cfg.addr, dst, g_gw.tx_seq, 0u);

    if (len < 0) return;

    g_gw.tx_seq++;
    rp_hw_if_send(buf, (size_t)len);
}

static void gw_try_deliver_mailbox(rp_node_entry_t *e)
{
    rp_msg_t pending;

    if (!rp_mailbox_take(&g_gw.mailbox, gw_node_index(e), &pending)) return;

    gw_send_msg(&pending, e->short_addr);
}

static void gw_handle_register(const rp_frame_t *f, uint32_t now_ms)
{
    rp_msg_t decoded;
    if (rp_msg_decode(f, &decoded) != RP_MSG_OK) return;

    uint16_t addr = gw_alloc_addr();
    rp_node_entry_t *e = rp_nodetab_register(
        &g_gw.nodetab, decoded.u.reg.dev_id, addr, 0u, now_ms, RP_IOT_KEEPALIVE_MS
    );

    rp_msg_t resp = {0};
    resp.type = RP_T_REG_RESP;
    resp.u.reg_resp.status       = e ? RP_OK : RP_E_NOMEM;
    resp.u.reg_resp.short_addr   = e ? addr : RP_ADDR_UNASSIGNED;
    resp.u.reg_resp.keepalive_s  = RP_IOT_KEEPALIVE_MS / 1000u;

    /* вузол ще не має короткої адреси — відповідь широкомовна, як і REGISTER */
    gw_send_msg(&resp, RP_ADDR_BROADCAST);
}

static void gw_handle_auth(const rp_frame_t *f, uint32_t now_ms)
{
    rp_msg_t decoded;
    if (rp_msg_decode(f, &decoded) != RP_MSG_OK) return;

    rp_node_entry_t *e = rp_nodetab_find_by_dev_id(&g_gw.nodetab, decoded.u.auth.dev_id);

    rp_msg_t resp = {0};
    resp.type = RP_T_AUTH_RESP;

    if (!e)
    {
        resp.u.auth_resp.status = RP_E_NOT_REGISTERED;
        gw_send_msg(&resp, f->hdr.src);
        return;
    }

    /* справжній challenge/response — криптографія, окреме ТЗ (§7) */
    e->session_id = now_ms;
    rp_nodetab_refresh_keepalive(e, now_ms, RP_IOT_KEEPALIVE_MS);

    resp.u.auth_resp.status     = RP_OK;
    resp.u.auth_resp.session_id = e->session_id;

    gw_send_msg(&resp, e->short_addr);
    gw_try_deliver_mailbox(e);
}

static void gw_handle_ping(const rp_frame_t *f, uint32_t now_ms)
{
    rp_msg_t decoded;
    if (rp_msg_decode(f, &decoded) != RP_MSG_OK) return;

    rp_node_entry_t *e = rp_nodetab_find_by_addr(&g_gw.nodetab, f->hdr.src);
    if (e) rp_nodetab_refresh_keepalive(e, now_ms, RP_IOT_KEEPALIVE_MS);

    rp_msg_t resp = {0};
    resp.type = RP_T_PONG;
    resp.u.ping.nonce = decoded.u.ping.nonce;
    gw_send_msg(&resp, f->hdr.src);

    if (e) gw_try_deliver_mailbox(e);
}

static void gw_handle_uplink_data(const rp_frame_t *f, uint32_t now_ms)
{
    rp_node_entry_t *e = rp_nodetab_find_by_addr(&g_gw.nodetab, f->hdr.src);
    if (!e) return; /* невідомий вузол — тихо ігноруємо */

    rp_nodetab_refresh_keepalive(e, now_ms, RP_IOT_KEEPALIVE_MS);

    if (!rp_nodetab_is_dup_seq(e, f->hdr.seq))
    {
        rp_nodetab_touch_seq(e, f->hdr.seq);
        /* декодовані дані (SENSOR/ALARM) прикладного споживача поки
         * немає в межах цього ТЗ — dedup+ACK тут, доставка вгору по
         * стеку залишена для інтеграційного шару. */
    }

    rp_msg_t ack = {0};
    ack.type = RP_T_ACK;
    ack.u.ack.tag        = f->core.tag;
    ack.u.ack.acked_type = f->hdr.type;
    ack.u.ack.status     = RP_OK;
    gw_send_msg(&ack, f->hdr.src);

    gw_try_deliver_mailbox(e);
}

static void gw_handle_ack(const rp_frame_t *f, uint32_t now_ms)
{
    rp_node_entry_t *e = rp_nodetab_find_by_addr(&g_gw.nodetab, f->hdr.src);
    if (e) rp_nodetab_refresh_keepalive(e, now_ms, RP_IOT_KEEPALIVE_MS);
}

static int gw_is_bulk_type(uint8_t type)
{
    switch (type)
    {
    case RP_T_SENSOR:
    case RP_T_EVENT:
    case RP_T_BULK_INIT:
    case RP_T_BULK_DATA:
    case RP_T_BULK_ACK:
    case RP_T_BULK_END:
    case RP_T_OTA_INIT:
    case RP_T_OTA_APPLY:
        return 1;
    default:
        return 0;
    }
}

static void gw_process_item(const rp_frameq_item_t *item, uint32_t now_ms)
{
    rp_frame_t f;

    f.hdr      = item->hdr;
    f.core     = item->core;
    f.tail     = item->tail;
    f.tail_len = item->tail_len;
    f.rssi     = item->rssi;
    f.snr      = item->snr;

    switch (f.hdr.type)
    {
    case RP_T_REGISTER: gw_handle_register(&f, now_ms); break;
    case RP_T_AUTH:      gw_handle_auth(&f, now_ms); break;
    case RP_T_PING:       gw_handle_ping(&f, now_ms); break;
    case RP_T_SENSOR:
    case RP_T_ALARM:      gw_handle_uplink_data(&f, now_ms); break;
    case RP_T_ACK:
    case RP_T_NACK:        gw_handle_ack(&f, now_ms); break;
    default:
        break;
    }
}

void rp_network_init(const rp_network_cfg_t *cfg)
{
    memset(&g_gw, 0, sizeof(g_gw));

    if (cfg)
    {
        g_gw.cfg = *cfg;
    }

    rp_nodetab_init(&g_gw.nodetab);
    rp_mailbox_init(&g_gw.mailbox);
    rp_frameq_init(&g_gw.urgent_q, RP_GW_URGENT_QUEUE_DEPTH);
    rp_frameq_init(&g_gw.bulk_q, RP_GW_BULK_QUEUE_DEPTH);

    rp_hw_if_select(g_gw.cfg.hw_if);
}

int rp_network_submit(const rp_msg_t *msg, rp_network_prio_t prio)
{
    (void)msg;
    (void)prio;

    /* контракт не несе адресата вузла — для gateway використовуйте
     * rp_iot_gateway_queue_downlink() (rp_iot_gateway.h). */
    return RP_MSG_E_UNKNOWN;
}

void rp_network_on_frame(const rp_frame_t *f, uint32_t now_ms)
{
    if (!f) return;

    (void)now_ms;

    rp_frameq_t *q = gw_is_bulk_type(f->hdr.type) ? &g_gw.bulk_q : &g_gw.urgent_q;
    rp_frameq_push(q, f); /* переповнення — кадр просто відкидається */
}

void rp_network_tick(uint32_t now_ms)
{
    rp_nodetab_expire(&g_gw.nodetab, now_ms);

    rp_frameq_item_t item;

    while (rp_frameq_pop(&g_gw.urgent_q, &item))
    {
        gw_process_item(&item, now_ms);
    }

    while (rp_frameq_pop(&g_gw.bulk_q, &item))
    {
        gw_process_item(&item, now_ms);
    }
}

uint32_t rp_network_next_deadline_ms(uint32_t now_ms)
{
    if (!rp_frameq_is_empty(&g_gw.urgent_q) || !rp_frameq_is_empty(&g_gw.bulk_q))
    {
        return 0u;
    }

    uint8_t  have    = 0u;
    uint32_t nearest = 0u;

    for (uint16_t i = 0; i < RP_MAX_NODES; i++)
    {
        const rp_node_entry_t *e = &g_gw.nodetab.entries[i];

        if (!e->in_use) continue;

        if (!have || (e->keepalive_deadline_ms < nearest))
        {
            nearest = e->keepalive_deadline_ms;
            have = 1u;
        }
    }

    if (!have) return RP_GW_IDLE_POLL_MS;

    return (nearest > now_ms) ? (nearest - now_ms) : 0u;
}

int rp_iot_gateway_queue_downlink(uint16_t short_addr, const rp_msg_t *msg)
{
    if (!msg) return RP_MSG_E_ARG;

    rp_node_entry_t *e = rp_nodetab_find_by_addr(&g_gw.nodetab, short_addr);
    if (!e) return RP_MSG_E_ARG;

    return rp_mailbox_put(&g_gw.mailbox, gw_node_index(e), msg);
}
