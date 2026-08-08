#include "os_task.h"
#include "main.h"

static void ethernet_task(void *arg)
{
    (void)arg;

    for (;;)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
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
