/* ==================================================================
 * rp_msg.h — прикладний шар над транспортом rp_proto
 *
 * Шар відповідає за одне: перетворення rp_msg_t (типобезпечна
 * структура в пам'яті) на пару core + tail і навпаки.
 *
 * Прикладний код НЕ повинен знати про TLV-теги. Якщо десь поза
 * rp_msg.c зустрічається rp_tlv_add_* — це помилка декомпозиції.
 * ================================================================== */
#ifndef RP_MSG_H
#define RP_MSG_H

#include "rp_proto.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Коди помилок цього шару.                                            */
/*                                                                     */
/* Іменовані окремо від rp_status_t (rp_proto.h): те — код, що їде     */
/* по ефіру в core.status; це — локальний результат кодера/декодера.  */
/* ------------------------------------------------------------------ */
typedef enum {
    RP_MSG_OK          =  0,
    RP_MSG_E_ARG       = -1,   /* NULL або нісенітні аргументи             */
    RP_MSG_E_UNKNOWN   = -2,   /* тип не зареєстровано в таблиці           */
    RP_MSG_E_OVERFLOW  = -3,   /* tail не вліз у RP_MAX_TAIL               */
    RP_MSG_E_MALFORMED = -4,   /* биті TLV або немає обов'язкового поля    */
    RP_MSG_E_TOO_MANY  = -5    /* елементів більше, ніж вміщає масив       */
} rp_msg_err_t;

/* ------------------------------------------------------------------ */
/* Реєстр TLV-тегів                                                    */
/*                                                                     */
/* 0x00..0x7F — теги протоколу, централізовано розподілені.            */
/* 0x80..0xFF — вендорські, гарантовано ніколи не будуть зайняті       */
/*              протоколом. Приймач мусить їх мовчки пропускати.       */
/* ------------------------------------------------------------------ */
typedef enum {
    RP_TLV_END         = 0x00,  /* термінатор, len=0 (опційно)         */

    /* дані */
    RP_TLV_READING     = 0x01,  /* rp_reading_t, 7 B                   */
    RP_TLV_TEXT        = 0x02,  /* UTF-8, БЕЗ завершального нуля       */
    RP_TLV_CHUNK       = 0x03,  /* сирий шматок bulk-передачі          */

    /* ідентифікація */
    RP_TLV_DEV_ID      = 0x10,  /* 8 B унікальний id заліза            */
    RP_TLV_HW_VER      = 0x11,  /* u16                                 */
    RP_TLV_FW_VER      = 0x12,  /* u32: major.minor.patch.build        */
    RP_TLV_CAPS        = 0x13,  /* u32 бітмапа можливостей             */
    RP_TLV_SHORT_ADDR  = 0x14,  /* u16, видана адреса                  */

    /* конфігурація */
    RP_TLV_PARAM_ID    = 0x20,  /* u16, іде перед PARAM_VAL            */
    RP_TLV_PARAM_VAL   = 0x21,  /* змінної довжини                     */

    /* статус */
    RP_TLV_UPTIME      = 0x30,  /* u32 секунд                          */
    RP_TLV_VBAT        = 0x31,  /* u16 мВ                              */
    RP_TLV_RSSI        = 0x32,  /* u8, зміщений: rssi_dbm + 128        */

    /* bulk / OTA */
    RP_TLV_CRC32       = 0x40,  /* u32                                 */
    RP_TLV_CHUNK_SIZE  = 0x41,  /* u16                                 */
    RP_TLV_BITMAP      = 0x42,  /* бітмапа отриманих чанків вікна      */

    /* час, помилки */
    RP_TLV_TIME_MS     = 0x50,  /* u16 мілісекунд (доповнює core.arg)  */
    RP_TLV_ERR_DETAIL  = 0x51,  /* текстовий опис помилки              */

    RP_TLV_VENDOR_BASE = 0x80
} rp_tlv_tag_t;

/* ------------------------------------------------------------------ */
/* Значення core.sub                                                   */
/*                                                                     */
/* sub інтерпретується ЗАВЖДИ в контексті type. Ті самі числові        */
/* значення в різних type означають різне — це навмисно.               */
/* ------------------------------------------------------------------ */

/* для RP_T_CFG_APP / RP_T_CFG_RADIO */
typedef enum {
    RP_CFG_GET   = 0x01,
    RP_CFG_SET   = 0x02,
    RP_CFG_RESET = 0x03,
    RP_CFG_SAVE  = 0x04    /* закомітити в NVM                        */
} rp_cfg_op_t;

/* ідентифікатори параметрів застосунку (rp_param_t.id) для RP_T_CFG_APP.
 * Це прикладний словник налаштувань, окремий від внутрішніх TLV-тегів
 * rp_msg.c — саме ці значення дозволено використовувати поза цим файлом. */
typedef enum {
    RP_PARAM_TX_INTERVAL_S = 0x01,  /* u32, період надсилання показань   */
    RP_PARAM_APP_MODE      = 0x02,  /* u8                                */
    RP_PARAM_NAME          = 0x03   /* рядок без \0                      */
} rp_app_param_id_t;

/* для RP_T_CTRL */
typedef enum {
    RP_CTRL_REBOOT   = 0x01,
    RP_CTRL_SLEEP    = 0x02,   /* core.arg = секунд                   */
    RP_CTRL_FACTORY  = 0x03,   /* core.arg = магічне число-запобіжник */
    RP_CTRL_TX_TEST  = 0x04,   /* core.arg = тривалість, мс           */
    RP_CTRL_STOP_TEST= 0x05
} rp_ctrl_act_t;

/* для RP_T_LOG */
typedef enum {
    RP_LOG_ERR   = 0x01,
    RP_LOG_WARN  = 0x02,
    RP_LOG_INFO  = 0x03,
    RP_LOG_DEBUG = 0x04
} rp_log_lvl_t;

/* для RP_T_EVENT / RP_T_ALARM */
typedef enum {
    RP_EV_BOOT       = 0x01,
    RP_EV_THRESHOLD  = 0x02,
    RP_EV_TAMPER     = 0x03,
    RP_EV_LOW_BAT    = 0x04,
    RP_EV_SENSOR_ERR = 0x05
} rp_ev_class_t;

/* ------------------------------------------------------------------ */
/* Ліміти                                                              */
/* ------------------------------------------------------------------ */
#define RP_DEV_ID_LEN      8u
#define RP_MAX_READINGS    8u
#define RP_MAX_PARAMS      6u
#define RP_MAX_PARAM_VAL   16u
#define RP_MAX_TEXT        48u
#define RP_MAX_BITMAP      16u

/* ------------------------------------------------------------------ */
/* Уніфікований запис вимірювання (тіло RP_TLV_READING, 7 байт)        */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  sensor_id;
    uint8_t  unit;       /* див. rp_unit_t */
    uint8_t  scale;      /* десятковий порядок: value * 10^(scale-128) */
    int32_t  value;
} rp_reading_t;

#define RP_READING_WIRE_SIZE 7u

typedef enum {
    RP_U_NONE = 0, RP_U_CELSIUS, RP_U_PERCENT_RH, RP_U_PASCAL, RP_U_VOLT,
    RP_U_AMPERE, RP_U_LUX, RP_U_METER, RP_U_PPM, RP_U_COUNT, RP_U_DBM
} rp_unit_t;

/* ------------------------------------------------------------------ */
/* Тіла повідомлень                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    uint16_t tag;          /* tag підтверджуваної транзакції           */
    uint8_t  acked_type;   /* який саме тип підтверджуємо              */
    uint8_t  status;       /* для NACK — причина                       */
} rp_m_ack_t;

typedef struct {
    uint32_t nonce;        /* PONG повертає його незмінним             */
} rp_m_ping_t;

typedef struct {
    uint32_t uptime_s;
    uint16_t vbat_mv;      /* 0 = не повідомляється                    */
    int8_t   rssi_dbm;
    uint8_t  status;
} rp_m_beacon_t;

typedef struct {
    uint8_t  dev_id[RP_DEV_ID_LEN];
    uint16_t hw_ver;
    uint32_t fw_ver;
    uint32_t caps;
} rp_m_register_t;

typedef struct {
    uint8_t  status;       /* 0 = прийнято                             */
    uint16_t short_addr;   /* видана адреса                            */
    uint32_t keepalive_s;  /* очікуваний період heartbeat              */
} rp_m_reg_resp_t;

typedef struct {
    uint8_t  dev_id[RP_DEV_ID_LEN];
    uint32_t response;     /* відповідь на виклик з попереднього REG_RESP/PING */
} rp_m_auth_t;

typedef struct {
    uint8_t  status;       /* 0 = автентифіковано                      */
    uint32_t session_id;   /* ідентифікатор сесії, видається шлюзом    */
} rp_m_auth_resp_t;

typedef struct {
    uint16_t id;
    uint8_t  len;                        /* 0 для GET                  */
    uint8_t  val[RP_MAX_PARAM_VAL];
} rp_param_t;

typedef struct {
    uint8_t    op;                       /* rp_cfg_op_t → core.sub     */
    uint8_t    status;                   /* тільки для CFG_RESP        */
    uint8_t    n;
    rp_param_t p[RP_MAX_PARAMS];
} rp_m_cfg_t;

typedef struct {
    uint8_t  dev_id[RP_DEV_ID_LEN];
    uint16_t hw_ver;
    uint32_t fw_ver;
    uint32_t uptime_s;
} rp_m_dev_info_t;

typedef struct {
    uint32_t epoch_s;
    uint16_t frac_ms;
} rp_m_time_t;

typedef struct {
    uint16_t      batch_tag;             /* → core.tag                 */
    uint8_t       n;
    rp_reading_t  r[RP_MAX_READINGS];
} rp_m_sensor_t;

typedef struct {
    uint8_t  ev_class;                   /* rp_ev_class_t → core.sub   */
    uint32_t ev_code;                    /* → core.arg                 */
    uint8_t  text_len;
    char     text[RP_MAX_TEXT];
} rp_m_event_t;

typedef struct {
    uint8_t  level;                      /* rp_log_lvl_t → core.sub    */
    uint32_t ts_ms;                      /* → core.arg                 */
    uint8_t  text_len;
    char     text[RP_MAX_TEXT];
} rp_m_log_t;

typedef struct {
    uint16_t xfer_tag;                   /* id передачі → core.tag     */
    uint32_t total_size;                 /* → core.arg                 */
    uint32_t crc32;
    uint16_t chunk_size;
    uint32_t fw_ver;                     /* тільки для OTA_INIT        */
} rp_m_bulk_init_t;

typedef struct {
    uint16_t xfer_tag;
    uint32_t offset;                     /* → core.arg                 */
    uint8_t  len;
    uint8_t  data[RP_MAX_TAIL > 64 ? 64 : RP_MAX_TAIL];
} rp_m_bulk_data_t;

typedef struct {
    uint16_t xfer_tag;
    uint32_t win_base;                   /* → core.arg                 */
    uint8_t  status;
    uint8_t  map_len;
    uint8_t  map[RP_MAX_BITMAP];
} rp_m_bulk_ack_t;

typedef struct {
    uint16_t xfer_tag;
    uint32_t total_size;
    uint32_t crc32;
    uint8_t  status;
} rp_m_bulk_end_t;

typedef struct {
    uint8_t  action;                     /* rp_ctrl_act_t → core.sub   */
    uint32_t arg;                        /* → core.arg                 */
} rp_m_ctrl_t;

typedef struct {
    uint8_t  status;                     /* код помилки → core.status  */
    uint16_t ref_tag;                    /* tag того, що не вдалось    */
    uint8_t  ref_type;                   /* → core.sub                 */
    uint8_t  text_len;
    char     text[RP_MAX_TEXT];
} rp_m_error_t;

/* сирий прохід: тіло не інтерпретується */
typedef struct {
    rp_core_t core;
    uint16_t  tail_len;
    uint8_t   tail[RP_MAX_TAIL];
} rp_m_raw_t;

/* ------------------------------------------------------------------ */
/* Теговане об'єднання                                                 */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t type;                        /* rp_type_t                  */
    union {
        rp_m_ack_t        ack;           /* ACK, NACK                  */
        rp_m_ping_t       ping;          /* PING, PONG                 */
        rp_m_beacon_t     beacon;        /* BEACON, HEARTBEAT          */
        rp_m_register_t   reg;
        rp_m_reg_resp_t   reg_resp;
        rp_m_auth_t       auth;
        rp_m_auth_resp_t  auth_resp;
        rp_m_cfg_t        cfg;           /* CFG_APP, CFG_RADIO, RESP   */
        rp_m_dev_info_t   dev_info;
        rp_m_time_t       time;
        rp_m_sensor_t     sensor;
        rp_m_event_t      event;         /* EVENT, ALARM               */
        rp_m_log_t        log;
        rp_m_bulk_init_t  bulk_init;     /* BULK_INIT, OTA_INIT        */
        rp_m_bulk_data_t  bulk_data;
        rp_m_bulk_ack_t   bulk_ack;
        rp_m_bulk_end_t   bulk_end;
        rp_m_ctrl_t       ctrl;          /* CTRL, OTA_APPLY            */
        rp_m_error_t      error;
        rp_m_raw_t        raw;           /* VENDOR, DISCOVER           */
    } u;
} rp_msg_t;

/* ------------------------------------------------------------------ */
/* Проміжна форма: те, що серіалізатор віддає транспорту               */
/* ------------------------------------------------------------------ */
typedef struct {
    rp_core_t core;
    uint16_t  tail_len;
    uint8_t   tail[RP_MAX_TAIL];
    uint8_t   flags;      /* флаги, яких вимагає САМ тип повідомлення  */
} rp_payload_t;

/* ------------------------------------------------------------------ */
/* API                                                                 */
/* ------------------------------------------------------------------ */

/* rp_msg_t → core + tail. Не чіпає ефір. */
int rp_msg_encode(const rp_msg_t *msg, rp_payload_t *out);

/* прийнятий кадр → rp_msg_t. Кадр має бути вже перевірений по CRC. */
int rp_msg_decode(const rp_frame_t *f, rp_msg_t *out);

/* encode + rp_build одним викликом. Повертає довжину кадру або <0. */
int rp_msg_build(uint8_t *out, size_t out_cap, const rp_msg_t *msg,
                 uint16_t src, uint16_t dst, uint8_t seq,
                 uint8_t extra_flags);

/* діагностика: ім'я типу для логів. Ніколи не повертає NULL. */
const char *rp_msg_type_name(uint8_t type);

/* чи вимагає цей тип підтвердження */
bool rp_msg_needs_ack(uint8_t type);

#ifdef __cplusplus
}
#endif
#endif /* RP_MSG_H */
