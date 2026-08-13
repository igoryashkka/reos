#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Джерело часу для шару network (rp_network.h) та App/. Всередині
 * network/, common/, port/ заборонено читати платформний час напряму
 * (xTaskGetTickCount/HAL_GetTick) — лише звідси. */
uint32_t rp_port_now_ms(void);

/* Сон на rp_network_next_deadline_ms(now) мілісекунд. На FreeRTOS —
 * реальний vTaskDelay; на хості (rp_port_host.c) — просування
 * симульованого годинника. */
void rp_port_sleep_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif
