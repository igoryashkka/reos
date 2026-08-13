#include "os_task.h"
#include "logger.h"
#include "main.h"
#include "config_pin.h"

static void ethernet_task(void *arg)
{
    (void)arg;

    uint32_t blink_count = 0;

    LOG_INFO(LOG_MODULE_ETH, "ethernet task online");

    for (;;)
    {
        HAL_GPIO_TogglePin(BSP_LED1_PORT, BSP_LED1_PIN);
        blink_count++;

        if ((blink_count % 10U) == 0U)
        {
            LOG_DEBUG(LOG_MODULE_ETH, "heartbeat blink_count=%lu", blink_count);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

OS_TASK_DEFINE(
    ethernet,
    ethernet_task,
    NULL,
    OS_PRIORITY_HIGH,
    1024
);
