#pragma once

#include "main.h"

/* ==================================================================
 * Централізовані визначення пінів плати H750VB — єдине джерело для
 * GPIO, які використовує прикладний/сервісний код (App/, Services/),
 * замість розкиданих літералів GPIOx/GPIO_PIN_x по файлах.
 *
 * MX_GPIO_Init() (Core/Src/main.c) лишається відповідальним за
 * clock-enable і HAL_GPIO_Init() згідно з .ioc, але тепер теж
 * посилається на символи звідси, а не на власні літерали.
 * ================================================================== */

/* User LED — PA1, push-pull output (Services/network/ethernet.c) */
#define BSP_LED1_PORT   GPIOA
#define BSP_LED1_PIN    GPIO_PIN_1

/* User button — PE3, EXTI rising edge (наразі лише сконфігурований,
 * обробник ще не підключено) */
#define BSP_BUTTON1_PORT  GPIOE
#define BSP_BUTTON1_PIN   GPIO_PIN_3

/* Logger UART (USART3) TX — PB10, AF7 (Core/Src/uart_stm32h750.c) */
#define BSP_LOG_UART_INSTANCE    USART3
#define BSP_LOG_UART_GPIO_PORT   GPIOB
#define BSP_LOG_UART_GPIO_PIN    GPIO_PIN_10
#define BSP_LOG_UART_GPIO_AF     GPIO_AF7_USART3
