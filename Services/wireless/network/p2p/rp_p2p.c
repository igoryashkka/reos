/* ==================================================================
 * rp_p2p.c — distributed peer-to-peer mesh (ТЗ §3.1, §9-15).
 *
 * TTL: заголовок rp_proto.h не має власного поля TTL/hop-count (усі
 * 12 байт зайняті) і його розширення поза дозволеним списком змін
 * (§30). Перевикористовуємо байт frag (задокументований як
 * фрагмент-індекс для bulk/OTA, який поза скопом цього ТЗ і для всіх
 * реалізованих типів наразі завжди 0xFF) — узгоджене рішення, без
 * жодних змін у rp_proto.* чи rp_msg.*.
 *
 * Ідентифікація сусіда для neighbor-таблиці: hdr.src лишається
 * end-to-end (незмінний по дорозі, лише frag/TTL зменшується при
 * ретрансляції — rp_forward.c) — тому апдейт neighbor-таблиці робимо
 * ЛИШЕ з кадрів, які за конструкцією завжди прямі, 1-хопові й ніколи
 * не форвардяться: PING/PONG (probe) і RP_T_VENDOR (route advertisement,
 * поле VENDOR обране бо воно єдине "opaque" в rp_proto.h — не має
 * дескриптора в rp_msg.c k_desc[], тож не зачіпає жоден спільний код).
 *
 * Forwarding policy (ТЗ §11, буквально):
 *   1. прямий сусід достатньої якості -> надіслати напряму
 *   2. відомий (не прострочений) маршрут -> надіслати
 *   3. інакше -> initiate_route_discovery() (flood-фолбек)
 * Крок 3 виконує ЛИШЕ джерело (rp_p2p_submit_to) — проміжний
 * ретранслятор, який не знає ні прямого сусіда, ні маршруту, мовчки
 * дропає (інакше кожен необізнаний вузол сліпо флудив би далі).
 * ================================================================== */
#include "rp_network.h"
#include "rp_p2p.h"

#include "rp_forward.h"
#include "rp_frameq.h"
#include "rp_hw_if.h"
#include "rp_neighbor.h"
#include "rp_route.h"

#include <string.h>

#ifndef RP_P2P_MAX_TTL
#define RP_P2P_MAX_TTL 8u
#endif

#ifndef RP_P2P_DIRECT_MIN_QUALITY
#define RP_P2P_DIRECT_MIN_QUALITY 60u
#endif

#ifndef RP_P2P_ROUTE_TIMEOUT_MS
#define RP_P2P_ROUTE_TIMEOUT_MS 60000u
#endif

#ifndef RP_P2P_NEIGHBOR_TIMEOUT_MS
#define RP_P2P_NEIGHBOR_TIMEOUT_MS 30000u
#endif

#ifndef RP_P2P_DISCOVERY_INTERVAL_MS
#define RP_P2P_DISCOVERY_INTERVAL_MS 5000u
#endif

#ifndef RP_P2P_ADVERT_INTERVAL_MS
#define RP_P2P_ADVERT_INTERVAL_MS 10000u
#endif

#ifndef RP_P2P_PENDING_TX_DEPTH
#define RP_P2P_PENDING_TX_DEPTH 4u
#endif

#define RP_P2P_ADVERT_MAX_ENTRIES 8u
#define RP_P2P_TX_CAP (RP_OVERHEAD + RP_MAX_PAYLOAD)

typedef struct {
    uint8_t  in_use;
    uint16_t destination;
    rp_msg_t msg;
} p2p_pending_tx_t;

typedef struct {
    rp_network_cfg_t     cfg;

    rp_neighbor_table_t  neighbors;
    rp_route_table_t     routes;
    rp_dup_cache_t        dup_cache;

    rp_frameq_t           urgent_q; /* PING/PONG/VENDOR — control-plane */
    rp_frameq_t           bulk_q;   /* усе інше — дані застосунку */

    p2p_pending_tx_t      pending_tx[RP_P2P_PENDING_TX_DEPTH];

    uint8_t   tx_seq;
    uint8_t   time_ready;

    uint32_t  next_discovery_ms;
    uint32_t  next_advert_ms;

    uint8_t   ping_pending;
    uint32_t  ping_nonce;
    uint32_t  ping_sent_ms;
} p2p_t;

static p2p_t g_p2p;

static void p2p_send_bytes(const uint8_t *buf, int len)
{
    if (len < 0) return;

    rp_hw_if_send(buf, (size_t)len);
}

/* Надсилає ГОТОВІ core+tail із заданими meta. Використовується і для
 * прямого/маршрутизованого шляху, і для control-plane кадрів. */
static void p2p_send_frame(const rp_tx_meta_t *meta, const rp_core_t *core, const uint8_t *tail, uint16_t tail_len)
{
    uint8_t buf[RP_P2P_TX_CAP];
    int len = rp_build(buf, sizeof(buf), meta, core, tail_len ? tail : NULL, tail_len);

    p2p_send_bytes(buf, len);
}

/* Політика §11/§14 для ВЛАСНОГО (щойно згенерованого тут) пакета:
 * прямий сусід > маршрут > flood-фолбек (крок 3 — лише для джерела). */
static void p2p_originate_and_send(
    uint16_t destination, uint32_t now_ms,
    const rp_tx_meta_t *meta, const rp_core_t *core, const uint8_t *tail, uint16_t tail_len
)
{
    rp_neighbor_t *direct = rp_neighbor_find(&g_p2p.neighbors, destination);

    if (direct && (rp_neighbor_quality(direct) >= RP_P2P_DIRECT_MIN_QUALITY))
    {
        p2p_send_frame(meta, core, tail, tail_len);
        return;
    }

    if (rp_route_find(&g_p2p.routes, destination, now_ms) != NULL)
    {
        p2p_send_frame(meta, core, tail, tail_len);
        return;
    }

    /* initiate_route_discovery(): немає кращого варіанту — флудимо самі */
    p2p_send_frame(meta, core, tail, tail_len);
}

static void p2p_send_ping_broadcast(uint32_t now_ms)
{
    rp_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = RP_T_PING;
    msg.u.ping.nonce = now_ms;

    rp_payload_t pl;
    if (rp_msg_encode(&msg, &pl) != RP_MSG_OK) return;

    rp_tx_meta_t meta;
    memset(&meta, 0, sizeof(meta));
    meta.type  = RP_T_PING;
    meta.flags = pl.flags;
    meta.seq   = g_p2p.tx_seq++;
    meta.src   = g_p2p.cfg.addr;
    meta.dst   = RP_ADDR_BROADCAST;
    meta.frag  = 0xFFu; /* discovery ping завжди 1-хоп, TTL не застосовний */

    p2p_send_frame(&meta, &pl.core, pl.tail_len ? pl.tail : NULL, pl.tail_len);

    g_p2p.ping_pending = 1u;
    g_p2p.ping_nonce    = msg.u.ping.nonce;
    g_p2p.ping_sent_ms  = now_ms;
}

static void p2p_send_pong(uint16_t dst, uint32_t nonce)
{
    rp_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = RP_T_PONG;
    msg.u.ping.nonce = nonce;

    rp_payload_t pl;
    if (rp_msg_encode(&msg, &pl) != RP_MSG_OK) return;

    rp_tx_meta_t meta;
    memset(&meta, 0, sizeof(meta));
    meta.type  = RP_T_PONG;
    meta.flags = pl.flags;
    meta.seq   = g_p2p.tx_seq++;
    meta.src   = g_p2p.cfg.addr;
    meta.dst   = dst;
    meta.frag  = 0xFFu;

    p2p_send_frame(&meta, &pl.core, pl.tail_len ? pl.tail : NULL, pl.tail_len);
}

/* RP_T_VENDOR: "opaque"-тег rp_proto.h без дескриптора в rp_msg.c —
 * тож тут кодуємо/декодуємо сирі байти напряму, обходячи TLV-кодек, і
 * не чіпаємо жоден спільний файл. Wire: [addr_hi][addr_lo][metric_hi]
 * [metric_lo] * N. Завжди 1-хоп broadcast (як PING) — ніколи не
 * форвардиться, тож поширюється по mesh лише переретрансляцією від
 * кожного вузла (класичний distance-vector). */
static void p2p_send_advert(uint32_t now_ms)
{
    uint8_t  tail[RP_P2P_ADVERT_MAX_ENTRIES * 4u];
    uint16_t n = 0u;

    for (uint16_t i = 0; (i < RP_P2P_MAX_NEIGHBORS) && (n < RP_P2P_ADVERT_MAX_ENTRIES); i++)
    {
        rp_neighbor_t *nb = &g_p2p.neighbors.entries[i];
        if (!nb->in_use) continue;

        uint16_t metric = rp_route_metric_default(nb, 1u);
        tail[n * 4u + 0u] = (uint8_t)(nb->addr >> 8);
        tail[n * 4u + 1u] = (uint8_t)nb->addr;
        tail[n * 4u + 2u] = (uint8_t)(metric >> 8);
        tail[n * 4u + 3u] = (uint8_t)metric;
        n++;
    }

    for (uint16_t i = 0; (i < RP_P2P_MAX_ROUTES) && (n < RP_P2P_ADVERT_MAX_ENTRIES); i++)
    {
        rp_route_t *r = &g_p2p.routes.entries[i];
        if (!r->in_use || (now_ms >= r->expires_ms)) continue;

        tail[n * 4u + 0u] = (uint8_t)(r->destination >> 8);
        tail[n * 4u + 1u] = (uint8_t)r->destination;
        tail[n * 4u + 2u] = (uint8_t)(r->metric >> 8);
        tail[n * 4u + 3u] = (uint8_t)r->metric;
        n++;
    }

    if (n == 0u) return;

    rp_core_t core;
    memset(&core, 0, sizeof(core));

    rp_tx_meta_t meta;
    memset(&meta, 0, sizeof(meta));
    meta.type  = RP_T_VENDOR;
    meta.flags = 0u;
    meta.seq   = g_p2p.tx_seq++;
    meta.src   = g_p2p.cfg.addr;
    meta.dst   = RP_ADDR_BROADCAST;
    meta.frag  = 0xFFu;

    p2p_send_frame(&meta, &core, tail, (uint16_t)(n * 4u));
}

static void p2p_handle_ping(const rp_frame_t *f, uint32_t now_ms)
{
    rp_msg_t decoded;
    if (rp_msg_decode(f, &decoded) != RP_MSG_OK) return;

    rp_neighbor_touch(&g_p2p.neighbors, f->hdr.src, now_ms, f->rssi, f->snr);
    p2p_send_pong(f->hdr.src, decoded.u.ping.nonce);
}

static void p2p_handle_pong(const rp_frame_t *f, uint32_t now_ms)
{
    rp_msg_t decoded;
    if (rp_msg_decode(f, &decoded) != RP_MSG_OK) return;

    rp_neighbor_t *n = rp_neighbor_touch(&g_p2p.neighbors, f->hdr.src, now_ms, f->rssi, f->snr);

    if (g_p2p.ping_pending && (decoded.u.ping.nonce == g_p2p.ping_nonce))
    {
        rp_neighbor_note_latency(n, (uint16_t)(now_ms - g_p2p.ping_sent_ms));
        g_p2p.ping_pending = 0u; /* фіксуємо latency з першої відповіді; решта — просто touch вище */
    }
}

static void p2p_handle_advert(const rp_frame_t *f, uint32_t now_ms)
{
    rp_neighbor_t *advertiser = rp_neighbor_touch(&g_p2p.neighbors, f->hdr.src, now_ms, f->rssi, f->snr);
    uint16_t       link_cost  = rp_route_metric_default(advertiser, 1u);

    uint16_t n = (uint16_t)(f->tail_len / 4u);

    for (uint16_t i = 0; i < n; i++)
    {
        const uint8_t *e = &f->tail[i * 4u];
        uint16_t dest    = (uint16_t)(((uint16_t)e[0] << 8) | e[1]);
        uint16_t metric  = (uint16_t)(((uint16_t)e[2] << 8) | e[3]);

        if (dest == g_p2p.cfg.addr) continue; /* маршрут до себе не потрібен */

        uint32_t candidate = (uint32_t)metric + link_cost;
        if (candidate > 0xFFFFu) candidate = 0xFFFFu;

        rp_route_upsert(&g_p2p.routes, dest, f->hdr.src, (uint16_t)candidate, now_ms, RP_P2P_ROUTE_TIMEOUT_MS);
    }
}

/* Дані застосунку (не PING/PONG/VENDOR), адресовані НЕ нам і не
 * broadcast — кандидат на ретрансляцію. Лише 2 гілки (без flood!, див.
 * коментар зверху файлу): проміжний вузол форвардить, тільки якщо
 * реально знає шлях. */
static void p2p_handle_forward_candidate(const rp_frame_t *f, uint32_t now_ms)
{
    if (rp_dup_cache_check_and_add(&g_p2p.dup_cache, f->hdr.src, f->hdr.seq, now_ms))
    {
        return; /* вже форвардили цей самий пакет */
    }

    rp_neighbor_t *direct = rp_neighbor_find(&g_p2p.neighbors, f->hdr.dst);
    int has_direct = direct && (rp_neighbor_quality(direct) >= RP_P2P_DIRECT_MIN_QUALITY);
    int has_route  = (rp_route_find(&g_p2p.routes, f->hdr.dst, now_ms) != NULL);

    if (!has_direct && !has_route)
    {
        return; /* немає відомого шляху — не флудимо навмання за чужий пакет */
    }

    uint8_t buf[RP_P2P_TX_CAP];
    int len = rp_forward_rebuild(buf, sizeof(buf), f, NULL);

    p2p_send_bytes(buf, len);
}

static void p2p_process_frame(const rp_frame_t *f, uint32_t now_ms)
{
    switch (f->hdr.type)
    {
    case RP_T_PING:
        if (f->hdr.dst == RP_ADDR_BROADCAST) p2p_handle_ping(f, now_ms);
        return;

    case RP_T_PONG:
        if (f->hdr.dst == g_p2p.cfg.addr) p2p_handle_pong(f, now_ms);
        return;

    case RP_T_VENDOR:
        if (f->hdr.dst == RP_ADDR_BROADCAST) p2p_handle_advert(f, now_ms);
        return;

    default:
        break;
    }

    if ((f->hdr.dst == g_p2p.cfg.addr) || (f->hdr.dst == RP_ADDR_BROADCAST))
    {
        /* дійшло до нас: прикладного споживача поки немає в межах цього
         * ТЗ (той самий підхід, що й у gateway для SENSOR/ALARM) */
        return;
    }

    p2p_handle_forward_candidate(f, now_ms);
}

static int p2p_is_control_type(uint8_t type)
{
    return (type == RP_T_PING) || (type == RP_T_PONG) || (type == RP_T_VENDOR);
}

static void p2p_drain_queue(rp_frameq_t *q, uint32_t now_ms)
{
    rp_frameq_item_t item;

    while (rp_frameq_pop(q, &item))
    {
        rp_frame_t f;
        f.hdr      = item.hdr;
        f.core     = item.core;
        f.tail     = item.tail;
        f.tail_len = item.tail_len;
        f.rssi     = item.rssi;
        f.snr      = item.snr;

        p2p_process_frame(&f, now_ms);
    }
}

static void p2p_drain_pending_tx(uint32_t now_ms)
{
    for (uint16_t i = 0; i < RP_P2P_PENDING_TX_DEPTH; i++)
    {
        p2p_pending_tx_t *pending = &g_p2p.pending_tx[i];
        if (!pending->in_use) continue;

        rp_payload_t pl;
        if (rp_msg_encode(&pending->msg, &pl) == RP_MSG_OK)
        {
            rp_tx_meta_t meta;
            memset(&meta, 0, sizeof(meta));
            meta.type  = pending->msg.type;
            meta.flags = pl.flags;
            meta.seq   = g_p2p.tx_seq++;
            meta.src   = g_p2p.cfg.addr;
            meta.dst   = pending->destination;
            meta.frag  = RP_P2P_MAX_TTL;

            p2p_originate_and_send(
                pending->destination, now_ms, &meta, &pl.core,
                pl.tail_len ? pl.tail : NULL, pl.tail_len
            );
        }

        pending->in_use = 0u;
    }
}

void rp_network_init(const rp_network_cfg_t *cfg)
{
    memset(&g_p2p, 0, sizeof(g_p2p));

    if (cfg)
    {
        g_p2p.cfg = *cfg;
    }

    rp_neighbor_init(&g_p2p.neighbors);
    rp_route_init(&g_p2p.routes);
    rp_dup_cache_init(&g_p2p.dup_cache);
    rp_frameq_init(&g_p2p.urgent_q, RP_FRAMEQ_DEPTH);
    rp_frameq_init(&g_p2p.bulk_q, RP_FRAMEQ_DEPTH);

    rp_hw_if_select(g_p2p.cfg.hw_if);
}

int rp_network_submit(const rp_msg_t *msg, rp_network_prio_t prio)
{
    (void)msg;
    (void)prio;

    /* контракт не несе адресата — використовуйте rp_p2p_submit_to()
     * (rp_p2p.h), як і rp_iot_gateway_queue_downlink() для gateway. */
    return RP_MSG_E_UNKNOWN;
}

int rp_p2p_submit_to(uint16_t destination, const rp_msg_t *msg, rp_network_prio_t prio)
{
    (void)prio; /* немає окремого urgent-шляху для P2P у межах цього ТЗ */

    if (!msg) return RP_MSG_E_ARG;

    for (uint16_t i = 0; i < RP_P2P_PENDING_TX_DEPTH; i++)
    {
        if (!g_p2p.pending_tx[i].in_use)
        {
            g_p2p.pending_tx[i].in_use      = 1u;
            g_p2p.pending_tx[i].destination = destination;
            g_p2p.pending_tx[i].msg         = *msg;
            return RP_MSG_OK;
        }
    }

    return RP_MSG_E_OVERFLOW;
}

void rp_network_on_frame(const rp_frame_t *f, uint32_t now_ms)
{
    if (!f) return;

    (void)now_ms;

    rp_frameq_t *q = p2p_is_control_type(f->hdr.type) ? &g_p2p.urgent_q : &g_p2p.bulk_q;
    rp_frameq_push(q, f); /* переповнення — кадр просто відкидається */
}

void rp_network_tick(uint32_t now_ms)
{
    if (!g_p2p.time_ready)
    {
        g_p2p.next_discovery_ms = now_ms + RP_P2P_DISCOVERY_INTERVAL_MS;
        g_p2p.next_advert_ms     = now_ms + RP_P2P_ADVERT_INTERVAL_MS;
        g_p2p.time_ready = 1u;
    }

    rp_neighbor_expire(&g_p2p.neighbors, now_ms, RP_P2P_NEIGHBOR_TIMEOUT_MS);
    rp_route_expire(&g_p2p.routes, now_ms);

    /* URGENT (control-plane) завжди перед BULK (дані/форвардинг), ТЗ §19/§26 */
    p2p_drain_queue(&g_p2p.urgent_q, now_ms);
    p2p_drain_queue(&g_p2p.bulk_q, now_ms);

    p2p_drain_pending_tx(now_ms);

    if (now_ms >= g_p2p.next_discovery_ms)
    {
        p2p_send_ping_broadcast(now_ms);
        g_p2p.next_discovery_ms = now_ms + RP_P2P_DISCOVERY_INTERVAL_MS;
    }

    if (now_ms >= g_p2p.next_advert_ms)
    {
        p2p_send_advert(now_ms);
        g_p2p.next_advert_ms = now_ms + RP_P2P_ADVERT_INTERVAL_MS;
    }
}

const rp_neighbor_t *rp_p2p_find_neighbor(uint16_t addr)
{
    return rp_neighbor_find(&g_p2p.neighbors, addr);
}

const rp_route_t *rp_p2p_find_route(uint16_t addr, uint32_t now_ms)
{
    return rp_route_find(&g_p2p.routes, addr, now_ms);
}

uint32_t rp_network_next_deadline_ms(uint32_t now_ms)
{
    if (!g_p2p.time_ready) return 0u;

    if (!rp_frameq_is_empty(&g_p2p.urgent_q) || !rp_frameq_is_empty(&g_p2p.bulk_q))
    {
        return 0u;
    }

    for (uint16_t i = 0; i < RP_P2P_PENDING_TX_DEPTH; i++)
    {
        if (g_p2p.pending_tx[i].in_use) return 0u;
    }

    uint32_t deadline = g_p2p.next_discovery_ms;
    if (g_p2p.next_advert_ms < deadline) deadline = g_p2p.next_advert_ms;

    return (deadline > now_ms) ? (deadline - now_ms) : 0u;
}
