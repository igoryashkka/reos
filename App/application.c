#include "logger.h"
#include "os_task.h"
#include "rp_network.h"
#include "rp_port.h"

#include "stm32h7xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <string.h>

#define APP_DEV_HW_VER  1u
#define APP_DEV_FW_VER  0x00010000u  /* 1.0.0.0 */
#define APP_DEV_CAPS    0u

/* App/ бачить лише rp_network.h — яка мережева модель (P2P/IoT) чи IoT
 * роль (leaf/router/gateway) насправді лінкується, обирає RP_NETWORK_MODEL/
 * RP_IOT_ROLE на етапі збірки (rp_config.cmake). Ані #ifdef, ані знання
 * про tx_buf/rp_msg_build/rp_hw_if_send тут бути не повинно. */
typedef struct
{
  uint8_t       dev_id[RP_DEV_ID_LEN];
  rp_reading_t  reading;
} application_t;

static application_t g_app;

static void application_init_data(void)
{
  uint32_t uid0 = HAL_GetUIDw0();
  uint32_t uid1 = HAL_GetUIDw1();

  g_app.dev_id[0] = (uint8_t)(uid0 >> 24);
  g_app.dev_id[1] = (uint8_t)(uid0 >> 16);
  g_app.dev_id[2] = (uint8_t)(uid0 >> 8);
  g_app.dev_id[3] = (uint8_t)uid0;
  g_app.dev_id[4] = (uint8_t)(uid1 >> 24);
  g_app.dev_id[5] = (uint8_t)(uid1 >> 16);
  g_app.dev_id[6] = (uint8_t)(uid1 >> 8);
  g_app.dev_id[7] = (uint8_t)uid1;

  g_app.reading.sensor_id = 1u;
  g_app.reading.unit      = RP_U_CELSIUS;
  g_app.reading.scale     = 127u;
  g_app.reading.value     = 250;
}

void application_init(void)
{
  rp_network_cfg_t cfg = {0};

  application_init_data();

  cfg.addr         = RP_ADDR_UNASSIGNED;
  cfg.gateway_addr = RP_ADDR_BROADCAST;
  memcpy(cfg.dev_id, g_app.dev_id, RP_DEV_ID_LEN);
  cfg.hw_ver = APP_DEV_HW_VER;
  cfg.fw_ver = APP_DEV_FW_VER;
  cfg.caps   = APP_DEV_CAPS;
  cfg.hw_if  = RP_HW_IF_UART;

  rp_network_init(&cfg);

  LOG_INFO(LOG_MODULE_APP, "application initialized");
}

void application_process(void)
{
  uint32_t now_ms = rp_port_now_ms();
  rp_msg_t msg = {0};

  msg.type = RP_T_SENSOR;
  msg.u.sensor.n = 1u;
  msg.u.sensor.r[0] = g_app.reading;
  g_app.reading.value++;

  if (rp_network_submit(&msg, RP_NETWORK_PRIO_NORMAL) != RP_MSG_OK)
  {
    LOG_ERROR(LOG_MODULE_APP, "submit failed");
  }

  rp_network_tick(now_ms);

  LOG_INFO(LOG_MODULE_APP, "tick now_ms=%lu", (unsigned long)now_ms);
}

static void application_task(void *arg)
{
  (void)arg;

  application_init();

  for (;;)
  {
    application_process();

    /* 0 = негайно є робота (наприклад, register/auth retry) — невеликий
     * floor замість зайнятого циклу без сну. */
    uint32_t deadline_ms = rp_network_next_deadline_ms(rp_port_now_ms());
    rp_port_sleep_ms((deadline_ms > 0u) ? deadline_ms : 10u);
  }
}

OS_TASK_DEFINE(
  application,
  application_task,
  NULL,
  OS_PRIORITY_NORMAL,
  1024
);
