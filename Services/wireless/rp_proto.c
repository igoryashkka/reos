#include "rp_proto.h"

static void rp_wr16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)(value & 0xFFu);
}

static void rp_wr32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)((value >> 16) & 0xFFu);
    out[2] = (uint8_t)((value >> 8) & 0xFFu);
    out[3] = (uint8_t)(value & 0xFFu);
}

static uint16_t rp_rd16(const uint8_t *in)
{
    return (uint16_t)(((uint16_t)in[0] << 8) | (uint16_t)in[1]);
}

static uint32_t rp_rd32(const uint8_t *in)
{
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) |
           ((uint32_t)in[2] << 8)  | (uint32_t)in[3];
}

static uint8_t rp_crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0x00u;

    for (size_t index = 0; index < length; index++)
    {
        crc ^= data[index];

        for (uint8_t bit = 0; bit < 8u; bit++)
        {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
        }
    }

    return crc;
}

static uint16_t rp_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFu;

    for (size_t index = 0; index < length; index++)
    {
        crc ^= (uint16_t)((uint16_t)data[index] << 8);

        for (uint8_t bit = 0; bit < 8u; bit++)
        {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
        }
    }

    return crc;
}


int rp_build(uint8_t *out, size_t out_cap,
             const rp_tx_meta_t *m, const rp_core_t *core,
             const uint8_t *tail, uint16_t tail_len)
{
    if (!out || !m || !core) return -1;
    if (tail_len > RP_MAX_TAIL) return -1;

    const uint16_t payload_len = (uint16_t)(RP_CORE_SIZE + tail_len);
    const size_t   total = RP_PREAMBLE_LEN + RP_SYNC_SIZE + RP_HDR_SIZE + payload_len + RP_CRC_SIZE;
    if (out_cap < total) return -1;

    size_t i = 0;
    for (unsigned k = 0; k < RP_PREAMBLE_LEN; k++) out[i++] = RP_PREAMBLE_BYTE;
    out[i++] = RP_SYNC0;
    out[i++] = RP_SYNC1;

    uint8_t *h = &out[i];                      /* початок області під CRC16 */
    h[RP_HDR_OFF_VERHLEN] = (uint8_t)((RP_VER << 4) | (RP_HDR_SIZE / 4u));
    h[RP_HDR_OFF_TYPE]    = m->type;
    h[RP_HDR_OFF_FLAGS]   = m->flags;
    h[RP_HDR_OFF_SEQ]     = m->seq;
    rp_wr16(&h[RP_HDR_OFF_SRC], m->src);
    rp_wr16(&h[RP_HDR_OFF_DST], m->dst);
    rp_wr16(&h[RP_HDR_OFF_LEN], payload_len);
    h[RP_HDR_OFF_FRAG]    = m->frag;
    h[RP_HDR_OFF_HCRC]    = rp_crc8(h, RP_HDR_SIZE - 1u);
    i += RP_HDR_SIZE;

    uint8_t *c = &out[i];
    c[RP_CORE_OFF_SUB]    = core->sub;
    c[RP_CORE_OFF_STATUS] = core->status;
    rp_wr16(&c[RP_CORE_OFF_TAG], core->tag);
    rp_wr32(&c[RP_CORE_OFF_ARG], core->arg);
    i += RP_CORE_SIZE;

    if (tail_len && tail) { memcpy(&out[i], tail, tail_len); i += tail_len; }

    uint16_t crc = rp_crc16(h, (size_t)(RP_HDR_SIZE + payload_len));
    rp_wr16(&out[i], crc);
    i += RP_CRC_SIZE;

    return (int)i;
}

void rp_parse_init(rp_rx_parser_t *p)
{
    if (!p) return;

    p->state       = RP_RX_STATE_SYNC0;
    p->index        = 0u;
    p->payload_len  = 0u;
    p->crc_index    = 0u;
}

static void rp_parse_resync(rp_rx_parser_t *p)
{
    p->state       = RP_RX_STATE_SYNC0;
    p->index        = 0u;
    p->payload_len  = 0u;
    p->crc_index    = 0u;
}

int rp_parse_byte(rp_rx_parser_t *p, uint8_t byte, rp_frame_t *out)
{
    if (!p || !out) return -1;

    switch (p->state)
    {
    case RP_RX_STATE_SYNC0:
        if (byte == RP_SYNC0)
        {
            p->state = RP_RX_STATE_SYNC1;
        }
        return 0;

    case RP_RX_STATE_SYNC1:
        if (byte == RP_SYNC1)
        {
            p->state = RP_RX_STATE_HDR;
            p->index  = 0u;
        }
        else if (byte != RP_SYNC0)
        {
            p->state = RP_RX_STATE_SYNC0;
        }
        /* byte == RP_SYNC0: лишаємось тут, це новий кандидат на sync0 */
        return 0;

    case RP_RX_STATE_HDR:
        p->buf[p->index++] = byte;
        if (p->index < RP_HDR_SIZE)
        {
            return 0;
        }

        {
            uint8_t  ver_hlen = p->buf[RP_HDR_OFF_VERHLEN];
            uint8_t  ver      = (uint8_t)(ver_hlen >> 4);
            uint8_t  hlen     = (uint8_t)((ver_hlen & 0x0Fu) * 4u);
            uint8_t  hcrc     = rp_crc8(p->buf, RP_HDR_SIZE - 1u);
            uint16_t len      = rp_rd16(&p->buf[RP_HDR_OFF_LEN]);

            if ((ver != RP_VER) || (hlen != RP_HDR_SIZE) ||
                (hcrc != p->buf[RP_HDR_OFF_HCRC]) ||
                (len < RP_CORE_SIZE) || (len > RP_MAX_PAYLOAD))
            {
                rp_parse_resync(p);
                return -1;
            }

            p->payload_len = len;
        }

        p->state = RP_RX_STATE_PAYLOAD;
        return 0;

    case RP_RX_STATE_PAYLOAD:
        p->buf[p->index++] = byte;
        if (p->index < (uint16_t)(RP_HDR_SIZE + p->payload_len))
        {
            return 0;
        }

        p->state    = RP_RX_STATE_CRC;
        p->crc_index = 0u;
        return 0;

    case RP_RX_STATE_CRC:
        p->crc_buf[p->crc_index++] = byte;
        if (p->crc_index < RP_CRC_SIZE)
        {
            return 0;
        }

        {
            uint16_t expected = rp_crc16(p->buf, (size_t)(RP_HDR_SIZE + p->payload_len));
            uint16_t received = rp_rd16(p->crc_buf);

            if (expected != received)
            {
                rp_parse_resync(p);
                return -1;
            }

            out->hdr.ver   = (uint8_t)(p->buf[RP_HDR_OFF_VERHLEN] >> 4);
            out->hdr.hlen  = RP_HDR_SIZE;
            out->hdr.type  = p->buf[RP_HDR_OFF_TYPE];
            out->hdr.flags = p->buf[RP_HDR_OFF_FLAGS];
            out->hdr.seq   = p->buf[RP_HDR_OFF_SEQ];
            out->hdr.src   = rp_rd16(&p->buf[RP_HDR_OFF_SRC]);
            out->hdr.dst   = rp_rd16(&p->buf[RP_HDR_OFF_DST]);
            out->hdr.len   = p->payload_len;
            out->hdr.frag  = p->buf[RP_HDR_OFF_FRAG];

            out->core.sub    = p->buf[RP_HDR_SIZE + RP_CORE_OFF_SUB];
            out->core.status = p->buf[RP_HDR_SIZE + RP_CORE_OFF_STATUS];
            out->core.tag    = rp_rd16(&p->buf[RP_HDR_SIZE + RP_CORE_OFF_TAG]);
            out->core.arg    = rp_rd32(&p->buf[RP_HDR_SIZE + RP_CORE_OFF_ARG]);

            out->tail     = &p->buf[RP_HDR_SIZE + RP_CORE_SIZE];
            out->tail_len = (uint16_t)(p->payload_len - RP_CORE_SIZE);
            out->rssi     = 0;
            out->snr      = 0;
        }

        rp_parse_resync(p);
        return 1;

    default:
        rp_parse_resync(p);
        return -1;
    }
}