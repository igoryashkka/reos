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