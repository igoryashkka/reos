#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================
 * Драйвер RFM66 — платформонезалежний: єдина залежність поза
 * стандартною бібліотекою — Platform/BSP/spi.h (портативний контракт
 * SPI-шини, реалізація якого per-board). Тут немає жодного #include
 * "main.h"/HAL — той самий rfm66.c компілюється й лінкується
 * незмінним і для H750VB, і для F413.
 * ================================================================== */

typedef enum {
    RFM66_OK        =  0,
    RFM66_E_ARG     = -1,
    RFM66_E_IO      = -2,
    RFM66_E_TIMEOUT = -3,
} rfm66_status_t;

typedef enum {
    RFM66_ADDR_FILTER_NONE           = 0,
    RFM66_ADDR_FILTER_NODE           = 1,
    RFM66_ADDR_FILTER_NODE_BROADCAST = 2,
} rfm66_addr_filter_t;

typedef enum {
    RFM66_SHAPING_NONE  = 0,
    RFM66_SHAPING_BT1_0 = 1,
    RFM66_SHAPING_BT0_5 = 2,
    RFM66_SHAPING_BT0_3 = 3,
} rfm66_shaping_t;

typedef enum {
    RFM66_DC_FREE_NONE       = 0,
    RFM66_DC_FREE_MANCHESTER = 1,
    RFM66_DC_FREE_WHITENING  = 2,
} rfm66_dc_free_t;

typedef struct {
    uint32_t            frequency_hz;
    uint32_t            bitrate_bps;
    uint32_t            deviation_hz;
    uint8_t              rx_bw_khz;       /* найближче табличне значення обирається всередині */
    uint8_t              afc_bw_khz;
    int8_t               power_dbm;       /* -18..+13 (без PA_BOOST/PA_DAC) */
    uint16_t             preamble_len;
    uint8_t              payload_len;     /* для fixed-length; ігнорується, якщо variable_length=1 */
    uint8_t              variable_length; /* 1 = variable-length пакети, 0 = fixed */
    uint8_t              crc_enabled;
    uint8_t              crc_auto_clear;
    rfm66_addr_filter_t  addr_filtering;
    uint8_t              node_address;      /* лише якщо addr_filtering != NONE */
    uint8_t              broadcast_address; /* лише якщо addr_filtering == NODE_BROADCAST */
    uint8_t              agc_enabled;
    uint8_t              afc_enabled;
    rfm66_shaping_t      modulation_shaping;
    rfm66_dc_free_t      dc_free;
} rfm66_config_t;

typedef struct {
    uint8_t ready;
} rfm66_t;

rfm66_status_t rfm66_init(rfm66_t *dev, const rfm66_config_t *cfg);
rfm66_status_t rfm66_transmit(rfm66_t *dev, const uint8_t *data, uint16_t length);
rfm66_status_t rfm66_receive(rfm66_t *dev, uint8_t *buf, uint16_t buf_cap, uint16_t *out_length);
int16_t        rfm66_read_rssi(rfm66_t *dev); /* дБм */

#ifdef __cplusplus
}
#endif
