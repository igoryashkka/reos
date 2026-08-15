#include "rfm66.h"
#include "rfm66_regs.h"
#include "spi.h"

#include <string.h>

/* ==================================================================
 * Кристал: FXOSC = 32 MHz (стандарт для всієї родини SX1231/RFM66/
 * RFM69) — якщо на реальній платі інший кварц, поправ FXOSC_HZ.
 * FSTEP — крок синтезатора, використовується і для Frf, і для Fdev
 * (той самий регістровий крок в обох).
 *
 * Довіра до значень:
 *  - FXOSC/FSTEP-формули (частота/бітрейт/девіація/RxBw/RSSI) —
 *    визначаються фізикою синтезатора, висока довіра.
 *  - Біт-поля нижче (RFM66_OPMODE_*, RFM66_PKT1_* тощо) — за
 *    конвенцією родини SX1231/RFM69, НЕ підтверджені за даташитом
 *    саме RFM66. Там, де невпевнені в значенні решти бітів регістру,
 *    навмисно read-modify-write замість повного перезапису байта.
 * ================================================================== */

#define RFM66_FXOSC_HZ   32000000u
#define RFM66_FSTEP_X1000 61035u /* FXOSC / 2^19, ×1000 щоб рахувати в цілих */

/* RegOpMode (0x01), біти 4:2 — режим. Порядок Sleep..Rx стабільний по
 * всій родині SX1231/RFM69. */
#define RFM66_OPMODE_MODE_MASK     (0x1Cu)
#define RFM66_OPMODE_MODE_SLEEP    (0x00u << 2)
#define RFM66_OPMODE_MODE_STANDBY  (0x01u << 2)
#define RFM66_OPMODE_MODE_FS       (0x02u << 2)
#define RFM66_OPMODE_MODE_TX       (0x03u << 2)
#define RFM66_OPMODE_MODE_RX       (0x04u << 2)

/* RegPaConfig (0x09): bit7=PaSelect (0=RFO pin), біти4:0=OutputPower.
 * RFO-шлях (без PA_BOOST/PA_DAC, тобто без +20дБм): Pout = -18 + OutputPower. */
#define RFM66_PACONFIG_PASELECT_RFO (0x00u)

/* RegPacketConfig1 (0x30) */
#define RFM66_PKT1_FORMAT_VARIABLE   (0x80u) /* bit7 = 1 → variable length */
#define RFM66_PKT1_DCFREE_SHIFT      5       /* біти6:5 */
#define RFM66_PKT1_DCFREE_MASK       (0x03u << RFM66_PKT1_DCFREE_SHIFT)
#define RFM66_PKT1_CRC_ON            (0x10u) /* bit4 */
#define RFM66_PKT1_CRC_AUTOCLEAR_OFF (0x08u) /* bit3 — див. коментар у rfm66_init() щодо полярності */
#define RFM66_PKT1_ADDR_FILT_SHIFT   1       /* біти2:1 */
#define RFM66_PKT1_ADDR_FILT_MASK    (0x03u << RFM66_PKT1_ADDR_FILT_SHIFT)

/* RegRxBw/RegAfcBw (0x12/0x13): біти5:3=Mantissa(00=16,01=20,10=24), біти2:0=Exponent.
 * RxBw = FXOSC / (Mantissa * 2^(Exponent+2)). Біти7:6 (DccFreq) НЕ чіпаємо
 * (read-modify-write) — невпевнені в оптимальному дефолті для RFM66. */
#define RFM66_RXBW_FIELD_MASK (0x3Fu) /* біти5:0 */

/* RegIrqFlags2 (0x3F) */
#define RFM66_IRQ2_PACKET_SENT  (0x08u) /* bit3 */
#define RFM66_IRQ2_PAYLOAD_READY (0x04u) /* bit2 */

#define RFM66_TX_TIMEOUT_POLLS  200000u /* грубий програмний timeout на опитування IRQ-прапорців */
#define RFM66_MAX_FIFO          66u     /* стандартний розмір FIFO для цієї родини чипів */

static uint8_t rfm66_reg_read(uint8_t addr)
{
    uint8_t tx[2] = { (uint8_t)(addr & 0x7Fu), 0x00u };
    uint8_t rx[2] = { 0 };

    bsp_spi_transfer(BSP_SPI_BUS_RADIO, tx, rx, sizeof(tx));

    return rx[1];
}

static void rfm66_reg_write(uint8_t addr, uint8_t value)
{
    uint8_t tx[2] = { (uint8_t)(addr | 0x80u), value };
    uint8_t rx[2];

    bsp_spi_transfer(BSP_SPI_BUS_RADIO, tx, rx, sizeof(tx));
}

static void rfm66_reg_write_masked(uint8_t addr, uint8_t mask, uint8_t value)
{
    uint8_t current = rfm66_reg_read(addr);

    rfm66_reg_write(addr, (uint8_t)((current & (uint8_t)~mask) | (value & mask)));
}

static void rfm66_set_mode(uint8_t mode)
{
    rfm66_reg_write_masked(RFM66_REG_OP_MODE, RFM66_OPMODE_MODE_MASK, mode);
}

/* FSTEP у мГц ×1000, тому: reg = round(hz / FSTEP) = round(hz*1000 / FSTEP_x1000) */
static uint32_t rfm66_hz_to_steps(uint32_t hz)
{
    uint64_t scaled = (uint64_t)hz * 1000u;

    return (uint32_t)((scaled + (RFM66_FSTEP_X1000 / 2u)) / RFM66_FSTEP_X1000);
}

static void rfm66_write_frf(uint32_t frequency_hz)
{
    uint32_t frf = rfm66_hz_to_steps(frequency_hz);

    rfm66_reg_write(RFM66_REG_FRF_MSB, (uint8_t)(frf >> 16));
    rfm66_reg_write(RFM66_REG_FRF_MID, (uint8_t)(frf >> 8));
    rfm66_reg_write(RFM66_REG_FRF_LSB, (uint8_t)frf);
}

static void rfm66_write_bitrate(uint32_t bitrate_bps)
{
    uint32_t reg = (bitrate_bps > 0u) ? (RFM66_FXOSC_HZ / bitrate_bps) : 0u;

    rfm66_reg_write(RFM66_REG_BITRATE_MSB, (uint8_t)(reg >> 8));
    rfm66_reg_write(RFM66_REG_BITRATE_LSB, (uint8_t)reg);
}

static void rfm66_write_deviation(uint32_t deviation_hz)
{
    uint32_t fdev = rfm66_hz_to_steps(deviation_hz) & 0x3FFFu; /* 14-бітне поле */

    rfm66_reg_write(RFM66_REG_FDEV_MSB, (uint8_t)(fdev >> 8));
    rfm66_reg_write(RFM66_REG_FDEV_LSB, (uint8_t)fdev);
}

/* Підбір Mantissa/Exponent для RxBw/AfcBw: перебір усіх 3*8 комбінацій,
 * обираємо найближчу до цілі — безпечніше за перепис таблиці з пам'яті,
 * бо спирається на ту саму формулу, що й даташит. */
static uint8_t rfm66_bw_reg(uint32_t target_hz)
{
    static const uint32_t mantissas[3] = { 16u, 20u, 24u };
    uint8_t  best_mant_code = 0u;
    uint8_t  best_exp = 0u;
    uint32_t best_diff = 0xFFFFFFFFu;

    for (uint8_t mant_code = 0; mant_code < 3u; mant_code++)
    {
        for (uint8_t exp = 0; exp < 8u; exp++)
        {
            uint32_t divider = mantissas[mant_code] * (1u << (exp + 2u));
            uint32_t bw_hz   = RFM66_FXOSC_HZ / divider;
            uint32_t diff    = (bw_hz > target_hz) ? (bw_hz - target_hz) : (target_hz - bw_hz);

            if (diff < best_diff)
            {
                best_diff      = diff;
                best_mant_code = mant_code;
                best_exp       = exp;
            }
        }
    }

    return (uint8_t)((best_mant_code << 3) | best_exp);
}

static void rfm66_write_power(int8_t power_dbm)
{
    int16_t clamped = power_dbm;

    if (clamped < -18) clamped = -18;
    if (clamped > 13)  clamped = 13;

    uint8_t out_power = (uint8_t)(clamped + 18); /* 0..31 */

    rfm66_reg_write(RFM66_REG_PA_CONFIG, (uint8_t)(RFM66_PACONFIG_PASELECT_RFO | (out_power & 0x1Fu)));
}

static void rfm66_write_packet_config(const rfm66_config_t *cfg)
{
    uint8_t pkt1 = 0u;

    if (cfg->variable_length) pkt1 |= RFM66_PKT1_FORMAT_VARIABLE;
    pkt1 |= (uint8_t)(((uint8_t)cfg->dc_free << RFM66_PKT1_DCFREE_SHIFT) & RFM66_PKT1_DCFREE_MASK);
    if (cfg->crc_enabled) pkt1 |= RFM66_PKT1_CRC_ON;
    /* crc_auto_clear=1 → "не чистити FIFO при поганому CRC" (за описом
     * користувача до RF_SetCrcAutoClear) → біт CrcAutoClearOff=1 */
    if (cfg->crc_auto_clear) pkt1 |= RFM66_PKT1_CRC_AUTOCLEAR_OFF;
    pkt1 |= (uint8_t)(((uint8_t)cfg->addr_filtering << RFM66_PKT1_ADDR_FILT_SHIFT) & RFM66_PKT1_ADDR_FILT_MASK);

    rfm66_reg_write(RFM66_REG_PACKET_CONFIG_1, pkt1);
    rfm66_reg_write(RFM66_REG_PAYLOAD_LENGTH, cfg->payload_len);

    if (cfg->addr_filtering != RFM66_ADDR_FILTER_NONE)
    {
        rfm66_reg_write(RFM66_REG_NODE_ADRS, cfg->node_address);
        rfm66_reg_write(RFM66_REG_BROADCAST_ADRS, cfg->broadcast_address);
    }
}

rfm66_status_t rfm66_init(rfm66_t *dev, const rfm66_config_t *cfg)
{
    if (!dev || !cfg) return RFM66_E_ARG;

    memset(dev, 0, sizeof(*dev));

    if (bsp_spi_init(BSP_SPI_BUS_RADIO) != 0) return RFM66_E_IO;

    bsp_spi_radio_reset(BSP_SPI_BUS_RADIO);

    /* Sanity-check: чіп відповідає на регістр VERSION. 0x00/0xFF —
     * типові "нічого не підключено" відповіді на MISO. */
    uint8_t version = rfm66_reg_read(RFM66_REG_VERSION);
    if ((version == 0x00u) || (version == 0xFFu)) return RFM66_E_IO;

    rfm66_set_mode(RFM66_OPMODE_MODE_STANDBY);

    rfm66_write_frf(cfg->frequency_hz);
    rfm66_write_bitrate(cfg->bitrate_bps);
    rfm66_write_deviation(cfg->deviation_hz);

    rfm66_reg_write_masked(RFM66_REG_RX_BW, RFM66_RXBW_FIELD_MASK, rfm66_bw_reg(cfg->rx_bw_khz * 1000u));
    rfm66_reg_write_masked(RFM66_REG_AFC_BW, RFM66_RXBW_FIELD_MASK, rfm66_bw_reg(cfg->afc_bw_khz * 1000u));

    rfm66_write_power(cfg->power_dbm);

    rfm66_reg_write(RFM66_REG_PREAMBLE_MSB, (uint8_t)(cfg->preamble_len >> 8));
    rfm66_reg_write(RFM66_REG_PREAMBLE_LSB, (uint8_t)cfg->preamble_len);

    rfm66_write_packet_config(cfg);

    /* TODO: AGC (RegRxConfig?)/AFC (RegAfcFei bit)/ModulationShaping */
    (void)cfg->agc_enabled;
    (void)cfg->afc_enabled;
    (void)cfg->modulation_shaping;

    dev->ready = 1u;

    return RFM66_OK;
}

rfm66_status_t rfm66_transmit(rfm66_t *dev, const uint8_t *data, uint16_t length)
{
    if (!dev || !dev->ready || !data || (length == 0u) || (length > RFM66_MAX_FIFO)) return RFM66_E_ARG;

    rfm66_set_mode(RFM66_OPMODE_MODE_STANDBY);

    uint8_t tx_header = (uint8_t)(RFM66_REG_FIFO | 0x80u);
    bsp_spi_transfer(BSP_SPI_BUS_RADIO, &tx_header, NULL, 1u);
    /* FIFO — послідовність окремих байтових транзакцій під CS: перша
     * встановлює адресу запису, bsp_spi_transfer() відпускає CS після
     * кожного виклику (див. Platform/BSP/spi.h), тож пишемо по байту,
     * тримаючи режим "адреса лише в першому байті" через повторний tx_header. */
    for (uint16_t i = 0; i < length; i++)
    {
        uint8_t byte_tx[2] = { tx_header, data[i] };
        bsp_spi_transfer(BSP_SPI_BUS_RADIO, byte_tx, NULL, 2u);
    }

    rfm66_set_mode(RFM66_OPMODE_MODE_TX);

    for (uint32_t poll = 0; poll < RFM66_TX_TIMEOUT_POLLS; poll++)
    {
        if ((rfm66_reg_read(RFM66_REG_IRQ_FLAGS_2) & RFM66_IRQ2_PACKET_SENT) != 0u)
        {
            rfm66_set_mode(RFM66_OPMODE_MODE_STANDBY);
            return RFM66_OK;
        }
    }

    rfm66_set_mode(RFM66_OPMODE_MODE_STANDBY);

    return RFM66_E_TIMEOUT;
}

rfm66_status_t rfm66_receive(rfm66_t *dev, uint8_t *buf, uint16_t buf_cap, uint16_t *out_length)
{
    if (!dev || !dev->ready || !buf || !out_length) return RFM66_E_ARG;

    *out_length = 0u;

    rfm66_set_mode(RFM66_OPMODE_MODE_RX);

    if ((rfm66_reg_read(RFM66_REG_IRQ_FLAGS_2) & RFM66_IRQ2_PAYLOAD_READY) == 0u)
    {
        return RFM66_OK; /* нічого не прийшло цього разу — не помилка */
    }

    uint16_t count = 0u;
    while (count < buf_cap)
    {
        uint8_t tx[2] = { RFM66_REG_FIFO & 0x7Fu, 0x00u };
        uint8_t rx[2] = { 0 };

        bsp_spi_transfer(BSP_SPI_BUS_RADIO, tx, rx, sizeof(tx));
        buf[count++] = rx[1];

        if ((rfm66_reg_read(RFM66_REG_IRQ_FLAGS_2) & RFM66_IRQ2_PAYLOAD_READY) == 0u) break;
    }

    rfm66_set_mode(RFM66_OPMODE_MODE_STANDBY);
    *out_length = count;

    return RFM66_OK;
}

int16_t rfm66_read_rssi(rfm66_t *dev)
{
    if (!dev || !dev->ready) return 0;

    uint8_t raw = rfm66_reg_read(RFM66_REG_RSSI_VALUE);

    /* RSSI(dBm) = -raw/2 — стандартна формула цієї родини чипів. */
    return (int16_t)(-((int16_t)raw) / 2);
}
