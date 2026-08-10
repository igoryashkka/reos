/* ==================================================================
 * rp_msg.c — серіалізація команд у payload
 * ================================================================== */
#include "rp_msg.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* TLV читання/запис — суто внутрішня деталь цього файлу.              */
/* Ніщо поза rp_msg.c не повинно знати про теги чи цю розкладку.       */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t *buf;
    uint16_t cap;
    uint16_t len;
    bool     ovf;
} rp_tlv_w_t;

static void rp_tlv_w_init(rp_tlv_w_t *w, uint8_t *buf, uint16_t cap)
{
    w->buf = buf;
    w->cap = cap;
    w->len = 0u;
    w->ovf = false;
}

static void rp_tlv_w_raw(rp_tlv_w_t *w, uint8_t tag, const uint8_t *data, uint8_t len)
{
    if (w->ovf) return;
    if ((uint16_t)(w->len + 2u + len) > w->cap) { w->ovf = true; return; }

    w->buf[w->len++] = tag;
    w->buf[w->len++] = len;
    if (len) { memcpy(&w->buf[w->len], data, len); }
    w->len = (uint16_t)(w->len + len);
}

static void rp_tlv_w_u16(rp_tlv_w_t *w, uint8_t tag, uint16_t v)
{
    uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)v };
    rp_tlv_w_raw(w, tag, b, 2u);
}

static void rp_tlv_w_u32(rp_tlv_w_t *w, uint8_t tag, uint32_t v)
{
    uint8_t b[4] = {
        (uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v
    };
    rp_tlv_w_raw(w, tag, b, 4u);
}

static void rp_tlv_w_bytes(rp_tlv_w_t *w, uint8_t tag, const uint8_t *data, uint8_t len)
{
    rp_tlv_w_raw(w, tag, data, len);
}

static void rp_tlv_w_reading(rp_tlv_w_t *w, const rp_reading_t *r)
{
    uint32_t uv = (uint32_t)r->value;
    uint8_t b[RP_READING_WIRE_SIZE] = {
        r->sensor_id, r->unit, r->scale,
        (uint8_t)(uv >> 24), (uint8_t)(uv >> 16), (uint8_t)(uv >> 8), (uint8_t)uv
    };
    rp_tlv_w_raw(w, RP_TLV_READING, b, RP_READING_WIRE_SIZE);
}

typedef struct {
    const uint8_t *buf;
    uint16_t       len;
    uint16_t       pos;
} rp_tlv_r_t;

static void rp_tlv_r_init(rp_tlv_r_t *r, const uint8_t *buf, uint16_t len)
{
    r->buf = buf;
    r->len = len;
    r->pos = 0u;
}

static bool rp_tlv_r_next(rp_tlv_r_t *r, uint8_t *tag, const uint8_t **data, uint8_t *len)
{
    uint8_t l;

    if ((uint16_t)(r->pos + 2u) > r->len) return false;
    l = r->buf[r->pos + 1u];
    if ((uint16_t)(r->pos + 2u + l) > r->len) return false;

    *tag  = r->buf[r->pos];
    *len  = l;
    *data = &r->buf[r->pos + 2u];
    r->pos = (uint16_t)(r->pos + 2u + l);
    return true;
}

static uint16_t rp_rd16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t rp_rd32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/* ------------------------------------------------------------------ */
/* Скорочення для кодерів                                              */
/*                                                                     */
/* rp_tlv_w_t має липкий прапорець ovf, тому проміжні виклики          */
/* rp_tlv_w_* перевіряти не треба — досить одного RP_W_END.            */
/* ------------------------------------------------------------------ */
#define RP_W_BEGIN(p)                                   \
    rp_tlv_w_t w;                                       \
    rp_tlv_w_init(&w, (p)->tail, RP_MAX_TAIL)

#define RP_W_END(p)                                     \
    do {                                                \
        if (w.ovf) return RP_MSG_E_OVERFLOW;            \
        (p)->tail_len = w.len;                          \
    } while (0)

/* обов'язковий TLV: якщо немає — кадр невалідний */
#define RP_REQ(expr)  do { if (!(expr)) return RP_MSG_E_MALFORMED; } while (0)


/* ================================================================== */
/* 0x0_  transport / service                                          */
/* ================================================================== */

static int enc_ack(const rp_msg_t *msg, rp_payload_t *payload)
{
    payload->core.tag    = msg->u.ack.tag;
    payload->core.sub    = msg->u.ack.acked_type;
    payload->core.status = msg->u.ack.status;
    payload->tail_len     = 0u;

    return RP_MSG_OK;
}

static int dec_ack(const rp_frame_t *frame, rp_msg_t *msg)
{
    msg->u.ack.tag        = frame->core.tag;
    msg->u.ack.acked_type = frame->core.sub;
    msg->u.ack.status     = frame->core.status;

    return RP_MSG_OK;
}

static int enc_ping(const rp_msg_t *msg, rp_payload_t *payload)
{
    payload->core.arg = msg->u.ping.nonce;
    payload->tail_len  = 0u;

    return RP_MSG_OK;
}

static int dec_ping(const rp_frame_t *frame, rp_msg_t *msg)
{
    msg->u.ping.nonce = frame->core.arg;

    return RP_MSG_OK;
}

/* ================================================================== */
/* 0x1_  session                                                       */
/* ================================================================== */

static int enc_register(const rp_msg_t *msg, rp_payload_t *payload)
{
    const rp_m_register_t *r = &msg->u.reg;

    RP_W_BEGIN(payload);
    rp_tlv_w_bytes(&w, RP_TLV_DEV_ID, r->dev_id, RP_DEV_ID_LEN);
    rp_tlv_w_u16(&w, RP_TLV_HW_VER, r->hw_ver);
    rp_tlv_w_u32(&w, RP_TLV_FW_VER, r->fw_ver);
    rp_tlv_w_u32(&w, RP_TLV_CAPS, r->caps);
    RP_W_END(payload);

    return RP_MSG_OK;
}

static int dec_register(const rp_frame_t *frame, rp_msg_t *msg)
{
    rp_m_register_t *r = &msg->u.reg;
    rp_tlv_r_t rd;
    uint8_t tag, len;
    const uint8_t *data;
    bool got_dev_id = false;

    memset(r, 0, sizeof *r);
    rp_tlv_r_init(&rd, frame->tail, frame->tail_len);
    while (rp_tlv_r_next(&rd, &tag, &data, &len))
    {
        switch (tag)
        {
        case RP_TLV_DEV_ID:
            RP_REQ(len == RP_DEV_ID_LEN);
            memcpy(r->dev_id, data, RP_DEV_ID_LEN);
            got_dev_id = true;
            break;
        case RP_TLV_HW_VER:
            RP_REQ(len == 2u);
            r->hw_ver = rp_rd16(data);
            break;
        case RP_TLV_FW_VER:
            RP_REQ(len == 4u);
            r->fw_ver = rp_rd32(data);
            break;
        case RP_TLV_CAPS:
            RP_REQ(len == 4u);
            r->caps = rp_rd32(data);
            break;
        default:
            break; /* невідомі теги мовчки пропускаємо */
        }
    }
    RP_REQ(got_dev_id);

    return RP_MSG_OK;
}

static int enc_reg_resp(const rp_msg_t *msg, rp_payload_t *payload)
{
    const rp_m_reg_resp_t *r = &msg->u.reg_resp;

    payload->core.status = r->status;
    payload->core.arg    = r->keepalive_s;

    RP_W_BEGIN(payload);
    rp_tlv_w_u16(&w, RP_TLV_SHORT_ADDR, r->short_addr);
    RP_W_END(payload);

    return RP_MSG_OK;
}

static int dec_reg_resp(const rp_frame_t *frame, rp_msg_t *msg)
{
    rp_m_reg_resp_t *r = &msg->u.reg_resp;
    rp_tlv_r_t rd;
    uint8_t tag, len;
    const uint8_t *data;

    memset(r, 0, sizeof *r);
    r->status      = frame->core.status;
    r->keepalive_s = frame->core.arg;

    rp_tlv_r_init(&rd, frame->tail, frame->tail_len);
    while (rp_tlv_r_next(&rd, &tag, &data, &len))
    {
        if (tag == RP_TLV_SHORT_ADDR)
        {
            RP_REQ(len == 2u);
            r->short_addr = rp_rd16(data);
        }
    }

    return RP_MSG_OK;
}

static int enc_auth(const rp_msg_t *msg, rp_payload_t *payload)
{
    const rp_m_auth_t *a = &msg->u.auth;

    payload->core.arg = a->response;

    RP_W_BEGIN(payload);
    rp_tlv_w_bytes(&w, RP_TLV_DEV_ID, a->dev_id, RP_DEV_ID_LEN);
    RP_W_END(payload);

    return RP_MSG_OK;
}

static int dec_auth(const rp_frame_t *frame, rp_msg_t *msg)
{
    rp_m_auth_t *a = &msg->u.auth;
    rp_tlv_r_t rd;
    uint8_t tag, len;
    const uint8_t *data;

    memset(a, 0, sizeof *a);
    a->response = frame->core.arg;

    rp_tlv_r_init(&rd, frame->tail, frame->tail_len);
    while (rp_tlv_r_next(&rd, &tag, &data, &len))
    {
        if (tag == RP_TLV_DEV_ID)
        {
            RP_REQ(len == RP_DEV_ID_LEN);
            memcpy(a->dev_id, data, RP_DEV_ID_LEN);
        }
    }

    return RP_MSG_OK;
}

static int enc_auth_resp(const rp_msg_t *msg, rp_payload_t *payload)
{
    const rp_m_auth_resp_t *a = &msg->u.auth_resp;

    payload->core.status = a->status;
    payload->core.arg    = a->session_id;
    payload->tail_len     = 0u;

    return RP_MSG_OK;
}

static int dec_auth_resp(const rp_frame_t *frame, rp_msg_t *msg)
{
    msg->u.auth_resp.status     = frame->core.status;
    msg->u.auth_resp.session_id = frame->core.arg;

    return RP_MSG_OK;
}

/* ================================================================== */
/* 0x2_  configuration ("стеінги")                                     */
/* ================================================================== */

static int enc_cfg(const rp_msg_t *msg, rp_payload_t *payload)
{
    const rp_m_cfg_t *c = &msg->u.cfg;

    RP_REQ(c->n <= RP_MAX_PARAMS);
    payload->core.sub    = c->op;
    payload->core.status = c->status;

    RP_W_BEGIN(payload);
    for (uint8_t i = 0; i < c->n; i++)
    {
        rp_tlv_w_u16(&w, RP_TLV_PARAM_ID, c->p[i].id);
        if (c->p[i].len)
        {
            rp_tlv_w_bytes(&w, RP_TLV_PARAM_VAL, c->p[i].val, c->p[i].len);
        }
    }
    RP_W_END(payload);

    return RP_MSG_OK;
}

static int dec_cfg(const rp_frame_t *frame, rp_msg_t *msg)
{
    rp_m_cfg_t *c = &msg->u.cfg;
    rp_tlv_r_t rd;
    uint8_t tag, len;
    const uint8_t *data;
    bool have_param = false;

    memset(c, 0, sizeof *c);
    c->op     = frame->core.sub;
    c->status = frame->core.status;

    rp_tlv_r_init(&rd, frame->tail, frame->tail_len);
    while (rp_tlv_r_next(&rd, &tag, &data, &len))
    {
        if (tag == RP_TLV_PARAM_ID)
        {
            RP_REQ(len == 2u);
            RP_REQ(c->n < RP_MAX_PARAMS);
            c->p[c->n].id  = rp_rd16(data);
            c->p[c->n].len = 0u;
            c->n++;
            have_param = true;
        }
        else if (tag == RP_TLV_PARAM_VAL)
        {
            RP_REQ(have_param);
            RP_REQ(len <= RP_MAX_PARAM_VAL);
            memcpy(c->p[c->n - 1].val, data, len);
            c->p[c->n - 1].len = len;
        }
    }

    return RP_MSG_OK;
}

/* ================================================================== */
/* 0x3_  data                                                          */
/* ================================================================== */

static int enc_sensor(const rp_msg_t *msg, rp_payload_t *payload)
{
    const rp_m_sensor_t *s = &msg->u.sensor;

    RP_REQ(s->n <= RP_MAX_READINGS);
    payload->core.tag = s->batch_tag;

    RP_W_BEGIN(payload);
    for (uint8_t i = 0; i < s->n; i++)
    {
        rp_tlv_w_reading(&w, &s->r[i]);
    }
    RP_W_END(payload);

    return RP_MSG_OK;
}

static int dec_sensor(const rp_frame_t *frame, rp_msg_t *msg)
{
    rp_m_sensor_t *s = &msg->u.sensor;
    rp_tlv_r_t rd;
    uint8_t tag, len;
    const uint8_t *data;

    memset(s, 0, sizeof *s);
    s->batch_tag = frame->core.tag;

    rp_tlv_r_init(&rd, frame->tail, frame->tail_len);
    while (rp_tlv_r_next(&rd, &tag, &data, &len))
    {
        if (tag == RP_TLV_READING)
        {
            RP_REQ(len == RP_READING_WIRE_SIZE);
            RP_REQ(s->n < RP_MAX_READINGS);
            s->r[s->n].sensor_id = data[0];
            s->r[s->n].unit      = data[1];
            s->r[s->n].scale     = data[2];
            s->r[s->n].value     = (int32_t)rp_rd32(&data[3]);
            s->n++;
        }
    }

    return RP_MSG_OK;
}

/* ================================================================== */
/* Таблиця дескрипторів                                                */
/*                                                                     */
/* Єдине місце, де тип повідомлення зв'язується зі своєю поведінкою.   */
/* Нова команда = один рядок тут + пара статичних функцій вище.        */
/* ================================================================== */

typedef int (*rp_enc_fn)(const rp_msg_t *, rp_payload_t *);
typedef int (*rp_dec_fn)(const rp_frame_t *, rp_msg_t *);

typedef struct {
    uint8_t     type;       /* rp_type_t */
    uint8_t     flags;      /* флаги, обов'язкові для цього типу       */
    rp_enc_fn   encode;
    rp_dec_fn   dec;
    const char *name;
} rp_desc_t;

static const rp_desc_t k_desc[] = {
    { RP_T_ACK,       0u,            enc_ack,       dec_ack,       "ACK"       },
    { RP_T_NACK,      0u,            enc_ack,       dec_ack,       "NACK"      },
    { RP_T_PING,      RP_F_ACK_REQ,  enc_ping,      dec_ping,      "PING"      },
    { RP_T_PONG,      0u,            enc_ping,      dec_ping,      "PONG"      },
    { RP_T_REGISTER,  RP_F_ACK_REQ,  enc_register,  dec_register,  "REGISTER"  },
    { RP_T_REG_RESP,  0u,            enc_reg_resp,  dec_reg_resp,  "REG_RESP"  },
    { RP_T_AUTH,      RP_F_ACK_REQ,  enc_auth,      dec_auth,      "AUTH"      },
    { RP_T_AUTH_RESP, 0u,            enc_auth_resp, dec_auth_resp, "AUTH_RESP" },
    { RP_T_CFG_APP,   RP_F_ACK_REQ,  enc_cfg,       dec_cfg,       "CFG_APP"   },
    { RP_T_CFG_RESP,  0u,            enc_cfg,       dec_cfg,       "CFG_RESP"  },
    { RP_T_SENSOR,    RP_F_ACK_REQ,  enc_sensor,    dec_sensor,    "SENSOR"    },
};

#define RP_NDESC (sizeof k_desc / sizeof k_desc[0])

static const rp_desc_t *find_desc(uint8_t type)
{
    for (size_t i = 0; i < RP_NDESC; i++)
    {
        if (k_desc[i].type == type) return &k_desc[i];
    }
    return NULL;
}
/* ================================================================== */


int rp_msg_encode(const rp_msg_t *msg, rp_payload_t *out)
{
    const rp_desc_t *desc;
    int rc;

    if (!msg || !out) return RP_MSG_E_ARG;

    desc = find_desc(msg->type);
    if (!desc) return RP_MSG_E_UNKNOWN;

    /* нульова ініціалізація критична: кодери заповнюють лише
       ті поля core, які використовують, решта мусить бути 0 */
    memset(out, 0, sizeof *out);
    out->flags = desc->flags;

    rc = desc->encode(msg, out);
    if (rc != RP_MSG_OK) return rc;

    return RP_MSG_OK;
}

int rp_msg_decode(const rp_frame_t *f, rp_msg_t *out)
{
    const rp_desc_t *desc;

    if (!f || !out) return RP_MSG_E_ARG;

    desc = find_desc(f->hdr.type);
    if (!desc) return RP_MSG_E_UNKNOWN;

    memset(out, 0, sizeof *out);
    out->type = f->hdr.type;

    return desc->dec(f, out);
}

int rp_msg_build(uint8_t *out, size_t out_cap, const rp_msg_t *msg,
                 uint16_t src, uint16_t dst, uint8_t seq,
                 uint8_t extra_flags)
{
    rp_payload_t pl;
    rp_tx_meta_t meta;
    int rc = rp_msg_encode(msg, &pl);
    if (rc != RP_MSG_OK) return rc;

    memset(&meta, 0, sizeof meta);
    meta.type  = msg->type;
    meta.flags = (uint8_t)(pl.flags | extra_flags);
    meta.seq   = seq;
    meta.src   = src;
    meta.dst   = dst;
    meta.frag  = 0xFFu;

    return rp_build(out, out_cap, &meta, &pl.core,
                    pl.tail_len ? pl.tail : NULL, pl.tail_len);
}

const char *rp_msg_type_name(uint8_t type)
{
    const rp_desc_t *desc = find_desc(type);
    return desc ? desc->name : "UNKNOWN";
}

bool rp_msg_needs_ack(uint8_t type)
{
    const rp_desc_t *desc = find_desc(type);
    return desc ? ((desc->flags & RP_F_ACK_REQ) != 0u) : false;
}
