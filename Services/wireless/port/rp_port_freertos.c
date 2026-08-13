#include "rp_port.h"

#include "FreeRTOS.h"
#include "task.h"

uint32_t rp_port_now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void rp_port_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
