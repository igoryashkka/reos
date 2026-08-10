#ifndef RP_PROTO_H
#define RP_PROTO_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RP_VER              1u
#define RP_PREAMBLE_BYTE    0xAAu
#define RP_PREAMBLE_LEN     4u      /* якщо радіо робить преамбулу апаратно — став 0 */
#define RP_SYNC0            0x5Au
#define RP_SYNC1            0xA5u
#define RP_SYNC_SIZE        2u
#define RP_HDR_SIZE         12u
#define RP_CORE_SIZE        8u
#define RP_CRC_SIZE         2u

#define RP_MAX_PAYLOAD      200u    /* CORE + TAIL; підганяй під MTU радіо */
#define RP_MAX_TAIL         (RP_MAX_PAYLOAD - RP_CORE_SIZE)
#define RP_FRAME_MAX        (RP_SYNC_SIZE + RP_HDR_SIZE + RP_MAX_PAYLOAD + RP_CRC_SIZE)
#define RP_OVERHEAD         (RP_PREAMBLE_LEN + RP_SYNC_SIZE + RP_HDR_SIZE + RP_CRC_SIZE)

#define RP_ADDR_BROADCAST   0xFFFFu
#define RP_ADDR_UNASSIGNED  0x0000u

/* ------------------------------------------------------------------ */
/* HEADER (12 байт)                                                    */
/*                                                                     */
/*  off size  поле                                                     */
/*   0   1    ver:4 | hlen:4   версія | довжина хідера в 4-байт словах  */
/*   1   1    type              ID команди                             */
/*   2   1    flags                                                    */
/*   3   1    seq               лічильник для ACK/дедуплікації          */
/*   4   2    src                                                      */
/*   6   2    dst                                                      */
/*   8   2    len               довжина PAYLOAD (CORE+TAIL)             */
/*  10   1    frag              індекс фрагмента (0..254), 0xFF = н/д   */
/*  11   1    hcrc              CRC8 над байтами 0..10                  */
/* ------------------------------------------------------------------ */
#define RP_HDR_OFF_VERHLEN  0u
#define RP_HDR_OFF_TYPE     1u
#define RP_HDR_OFF_FLAGS    2u
#define RP_HDR_OFF_SEQ      3u
#define RP_HDR_OFF_SRC      4u
#define RP_HDR_OFF_DST      6u
#define RP_HDR_OFF_LEN      8u
#define RP_HDR_OFF_FRAG     10u
#define RP_HDR_OFF_HCRC     11u

/* ---- flags ---- */
#define RP_F_ACK_REQ    0x01u   /* відправник чекає ACK                     */
#define RP_F_ACK        0x02u   /* цей кадр — підтвердження                 */
#define RP_F_NACK       0x04u   /* негативне підтвердження (див. core.status)*/
#define RP_F_FRAG       0x08u   /* частина фрагментованої передачі          */
#define RP_F_LAST       0x10u   /* останній фрагмент                        */
#define RP_F_ENC        0x20u   /* payload зашифрований (окрім CORE.tag)    */
#define RP_F_MIC        0x40u   /* у хвості є TLV_MIC                       */
#define RP_F_RETRY      0x80u   /* повторна передача                        */


typedef enum {
    /* 0x0_ transport / service */
    RP_T_ACK          = 0x01,  /* acknowledgment, core.tag = tag of the acknowledged */
    RP_T_NACK         = 0x02,  /* negative acknowledgment, core.status = reason              */
    RP_T_PING         = 0x03,  /* with RP_F_ACK_REQ — requires a response          */
    RP_T_PONG         = 0x04,
    RP_T_BEACON       = 0x05,  /* ping without ACK, broadcast, "I'm alive"          */
    RP_T_HEARTBEAT    = 0x06,  /* periodic keepalive with mini-status       */

    /* 0x1_ Sessions cmds  */
    RP_T_DISCOVER     = 0x10,  /* gateway discovery                                 */
    RP_T_REGISTER     = 0x11,  /* registration: dev_id, hw/fw, capabilities     */
    RP_T_REG_RESP     = 0x12,  /* issuance of a short address + parameters         */
    RP_T_AUTH         = 0x13,  /* challenge/response                          */
    RP_T_AUTH_RESP    = 0x14,
    //RP_T_REKEY        = 0x15,  /* change of the session key                       */
    //RP_T_DEAUTH       = 0x16,  /* exit from the network / session invalidation         */

    /* 0x2_ Configuration */
    RP_T_CFG_APP      = 0x20,  /* configuration of the app (get/set in core.sub)  */
    RP_T_CFG_RADIO    = 0x21,  /* configuration of the radio                          */
    RP_T_CFG_RESP     = 0x22,  /* response to any CFG                  */
    RP_T_DEV_INFO     = 0x23,  /* device serial number, versions, uptime                    */
    RP_T_TIME_SYNC    = 0x24,  /* time synchronization                          */

    /* 0x3_ Data */
    RP_T_SENSOR       = 0x30,  /* sensor telemetry                         */
    RP_T_EVENT        = 0x31,  /* asynchronous event                            */
    RP_T_ALARM        = 0x32,  /* alarm, always with ACK_REQ                    */
    RP_T_LOG          = 0x33,  /* text log                                   */

    /* 0x4_ Bulk transfers */
    RP_T_BULK_INIT    = 0x40,  /* request: total_size, crc32, chunk_size       */
    RP_T_BULK_DATA    = 0x41,  /* core.arg = offset, tail = TLV_CHUNK        */
    RP_T_BULK_ACK     = 0x42,  /* bitmap of received chunks in the window */
    RP_T_BULK_END     = 0x43,  /* end + final CRC32 check                      */
    RP_T_OTA_INIT     = 0x44,  /* OTA separate bulk with firmware semantics     */
    RP_T_OTA_APPLY    = 0x45,

    /* 0x7_ Control */
    RP_T_CTRL         = 0x70,  /* core.sub: reboot / sleep / factory / tx-test */

    /* 0xF_ */
    RP_T_ERROR        = 0xF0,
    RP_T_VENDOR       = 0xFF
} rp_type_t;


/* Значення core.sub для CFG та CTRL команд — це семантика вмісту, тому
 * вони визначені в rp_msg.h (rp_cfg_op_t, rp_ctrl_act_t), а не тут. */

typedef enum {
    RP_OK              = 0x00,
    RP_E_UNKNOWN_CMD   = 0x01,
    RP_E_BAD_PARAM     = 0x02,
    RP_E_AUTH          = 0x03,
    RP_E_BUSY          = 0x04,
    RP_E_NOMEM         = 0x05,
    RP_E_CRC           = 0x06,
    RP_E_SEQ           = 0x07,
    RP_E_UNSUPPORTED   = 0x08,
    RP_E_TIMEOUT       = 0x09,
    RP_E_RANGE         = 0x0A,
    RP_E_NOT_REGISTERED= 0x0B,
    RP_E_INTERNAL      = 0xFF
} rp_status_t;

typedef struct {
    uint8_t  type;
    uint8_t  flags;
    uint8_t  seq;
    uint16_t src;
    uint16_t dst;
    uint8_t  frag;
} rp_tx_meta_t;

/* TLV tags, reading/unit types, and every other payload-shape concern
 * live in rp_msg.h — the transport layer only moves opaque bytes and
 * must not know how the tail is structured. */

/* ------------------------------------------------------------------ */
/* Розібраний кадр (в RAM, не packed — жодних unaligned читань)        */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  ver;
    uint8_t  hlen;      /* байт */
    uint8_t  type;
    uint8_t  flags;
    uint8_t  seq;
    uint16_t src;
    uint16_t dst;
    uint16_t len;       /* довжина payload */
    uint8_t  frag;
} rp_hdr_t;

/* Фіксоване ядро пейлоуда — однакове для ВСІХ команд */
typedef struct {
    uint8_t  sub;       /* під-операція в межах type                 */
    uint8_t  status;    /* rp_status_t; у запитах = 0                */
    uint16_t tag;       /* id транзакції: запит<->відповідь<->ACK    */
    uint32_t arg;       /* універсальний аргумент: offset/ts/session */
} rp_core_t;
#define RP_CORE_OFF_SUB     0u
#define RP_CORE_OFF_STATUS  1u
#define RP_CORE_OFF_TAG     2u
#define RP_CORE_OFF_ARG     4u

typedef struct {
    rp_hdr_t       hdr;
    rp_core_t      core;
    const uint8_t *tail;      /* вказує всередину буфера парсера */
    uint16_t       tail_len;
    int8_t         rssi;      /* заповнює радіо-драйвер */
    int8_t         snr;
} rp_frame_t;

int rp_build(uint8_t *out, size_t out_cap,
             const rp_tx_meta_t *m, const rp_core_t *core,
             const uint8_t *tail, uint16_t tail_len);

#ifdef __cplusplus
}
#endif

#endif /* RP_PROTO_H */
