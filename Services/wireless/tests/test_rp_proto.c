/* rp_build() <-> rp_parse_byte() round-trip, resync-after-junk,
 * bad-CRC rejection+recovery, and the RP_T_ALARM encoder added for
 * roles/rp_role_leaf.c's urgent path (rp_msg.c had no encoder for it
 * before this TZ — see PR description). */
#include "rp_msg.h"
#include "rp_proto.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_build_parse_roundtrip(void)
{
    uint8_t frame[RP_FRAME_MAX];
    rp_tx_meta_t m = {
        .type = RP_T_PING, .flags = RP_F_ACK_REQ, .seq = 7,
        .src = 0x1234, .dst = 0x5678, .frag = 0xFF
    };
    rp_core_t core = { .sub = 1, .status = 0, .tag = 0xBEEF, .arg = 0xDEADBEEFu };
    uint8_t tail[5] = {1, 2, 3, 4, 5};

    int len = rp_build(frame, sizeof(frame), &m, &core, tail, sizeof(tail));
    assert(len > 0);

    rp_rx_parser_t p;
    rp_frame_t out;
    rp_parse_init(&p);

    int result = 0;
    for (int i = 0; i < len; i++)
    {
        result = rp_parse_byte(&p, frame[i], &out);
        assert((result == 0) || (i == len - 1));
    }
    assert(result == 1);

    assert(out.hdr.type == RP_T_PING);
    assert(out.hdr.flags == RP_F_ACK_REQ);
    assert(out.hdr.seq == 7);
    assert(out.hdr.src == 0x1234);
    assert(out.hdr.dst == 0x5678);
    assert(out.core.sub == 1);
    assert(out.core.tag == 0xBEEF);
    assert(out.core.arg == 0xDEADBEEFu);
    assert(out.tail_len == 5);
    assert(memcmp(out.tail, tail, 5) == 0);

    /* junk before sync must not desync the parser */
    rp_parse_init(&p);
    uint8_t junk[3] = {0x00, 0x5A, 0x11};
    for (int i = 0; i < 3; i++) rp_parse_byte(&p, junk[i], &out);
    result = 0;
    for (int i = 0; i < len; i++) result = rp_parse_byte(&p, frame[i], &out);
    assert(result == 1);

    /* corrupted CRC is rejected... */
    rp_parse_init(&p);
    uint8_t bad[RP_FRAME_MAX];
    memcpy(bad, frame, (size_t)len);
    bad[len - 1] ^= 0xFF;
    result = 0;
    for (int i = 0; i < len; i++) result = rp_parse_byte(&p, bad[i], &out);
    assert(result == -1);

    /* ...and the parser recovers for the next good frame */
    result = 0;
    for (int i = 0; i < len; i++) result = rp_parse_byte(&p, frame[i], &out);
    assert(result == 1);

    printf("test_build_parse_roundtrip OK\n");
}

static void test_alarm_encoder(void)
{
    rp_msg_t msg = {0};
    msg.type = RP_T_ALARM;
    msg.u.event.ev_class = 3;
    msg.u.event.ev_code = 0xCAFEBABEu;
    const char *txt = "overheat";
    msg.u.event.text_len = (uint8_t)strlen(txt);
    memcpy(msg.u.event.text, txt, msg.u.event.text_len);

    uint8_t buf[RP_FRAME_MAX];
    int len = rp_msg_build(buf, sizeof(buf), &msg, 0x0001, 0x0000, 9, 0);
    assert(len > 0);

    rp_rx_parser_t p;
    rp_frame_t f;
    rp_parse_init(&p);
    int result = 0;
    for (int i = 0; i < len; i++) result = rp_parse_byte(&p, buf[i], &f);
    assert(result == 1);
    assert(f.hdr.flags & RP_F_ACK_REQ);

    rp_msg_t decoded;
    assert(rp_msg_decode(&f, &decoded) == RP_MSG_OK);
    assert(decoded.type == RP_T_ALARM);
    assert(decoded.u.event.ev_class == 3);
    assert(decoded.u.event.ev_code == 0xCAFEBABEu);
    assert(decoded.u.event.text_len == msg.u.event.text_len);
    assert(memcmp(decoded.u.event.text, txt, decoded.u.event.text_len) == 0);
    assert(rp_msg_needs_ack(RP_T_ALARM) == true);

    printf("test_alarm_encoder OK\n");
}

int main(void)
{
    test_build_parse_roundtrip();
    test_alarm_encoder();
    printf("ALL rp_proto/rp_msg TESTS OK\n");
    return 0;
}
