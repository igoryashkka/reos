#include "logger.h"
#include "os_task.h"
#include "rp_hw_if.h"
#include "rp_link.h"
#include "rp_link_auth.h"
#include "rp_link_ping.h"
#include "rp_link_register.h"
#include "rp_link_sensor.h"
#include "rp_link_settings.h"
#include "rp_msg.h"
#include "stm32h7xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define APP_DEV_HW_VER  1u
#define APP_DEV_FW_VER  0x00010000u  /* 1.0.0.0 */
#define APP_DEV_CAPS    0u

/* Послідовність роботи застосунку: реєстрація -> автентифікація ->
 * налаштування -> робочий цикл (пінг чергується з надсиланням даних). */
typedef enum
{
  APP_STAGE_REGISTER = 0,
  APP_STAGE_AUTH,
  APP_STAGE_SETTINGS,
  APP_STAGE_RUN_PING,
  APP_STAGE_RUN_SENSOR
} application_stage_t;

typedef struct
{
  rp_link_t            link;
  application_stage_t  stage;
  uint8_t              dev_id[RP_DEV_ID_LEN];
  rp_reading_t         reading;
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
  rp_hw_if_init_all();
  rp_link_init(&g_app.link);
  application_init_data();
  g_app.stage = APP_STAGE_REGISTER;

  LOG_INFO(LOG_MODULE_APP, "application initialized");
}

void application_process(void)
{
  uint8_t tx_buf[RP_LINK_TX_CAP];
  int tx_len;

  switch (g_app.stage)
  {
  case APP_STAGE_REGISTER:
    tx_len = rp_link_build_register(
      &g_app.link, tx_buf, sizeof(tx_buf),
      g_app.dev_id, APP_DEV_HW_VER, APP_DEV_FW_VER, APP_DEV_CAPS
    );
    if (tx_len >= 0)
    {
      g_app.stage = APP_STAGE_AUTH;
    }
    break;

  case APP_STAGE_AUTH:
    tx_len = rp_link_build_auth(
      &g_app.link, tx_buf, sizeof(tx_buf),
      g_app.dev_id, (uint32_t)xTaskGetTickCount()
    );
    if (tx_len >= 0)
    {
      g_app.stage = APP_STAGE_SETTINGS;
    }
    break;

  case APP_STAGE_SETTINGS:
    {
      rp_param_t tx_interval = { .id = RP_PARAM_TX_INTERVAL_S, .len = 0u };

      tx_len = rp_link_build_settings(
        &g_app.link, tx_buf, sizeof(tx_buf), &tx_interval, 1u, false
      );
    }
    if (tx_len >= 0)
    {
      g_app.stage = APP_STAGE_RUN_PING;
    }
    break;

  case APP_STAGE_RUN_PING:
    tx_len = rp_link_build_ping(
      &g_app.link, tx_buf, sizeof(tx_buf), (uint32_t)xTaskGetTickCount()
    );
    g_app.stage = APP_STAGE_RUN_SENSOR;
    break;

  case APP_STAGE_RUN_SENSOR:
  default:
    tx_len = rp_link_build_sensor(
      &g_app.link, tx_buf, sizeof(tx_buf), &g_app.reading, 1u
    );
    g_app.reading.value++;
    g_app.stage = APP_STAGE_RUN_PING;
    break;
  }

  if (tx_len < 0)
  {
    LOG_ERROR(LOG_MODULE_APP, "build failed stage=%d", (int)g_app.stage);
    return;
  }

  LOG_INFO(LOG_MODULE_APP, "built len=%d stage=%d", tx_len, (int)g_app.stage);

  if (rp_hw_if_send(tx_buf, (size_t)tx_len) < 0)
  {
    LOG_ERROR(LOG_MODULE_APP, "hw send failed stage=%d", (int)g_app.stage);
  }
}

static void application_task(void *arg)
{
  (void)arg;

  application_init();

  for (;;)
  {
    application_process();
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

OS_TASK_DEFINE(
  application,
  application_task,
  NULL,
  OS_PRIORITY_NORMAL,
  1024
);
