#pragma once

#include "main.h"

/* ==================================================================
 * Централізовані визначення пінів плати F413 — той самий підхід, що й
 * Platform/BSP/H750VB/config_pin.h. Призначення GPIO_Output/EXTI пінів
 * у F413.ioc ніяк не марковане (немає custom-labels), тож імена тут
 * навмисно узагальнені (OUTn/EXTIn), а не вигадані під конкретне
 * периферійне призначення, доки воно не підтверджене на платі.
 * ================================================================== */

/* SPI1..SPI5 — усі SPI_NSS_SOFT (CS керується вручну через OUTn нижче) */
#define BSP_SPI1_INSTANCE   SPI1
#define BSP_SPI2_INSTANCE   SPI2
#define BSP_SPI3_INSTANCE   SPI3
#define BSP_SPI4_INSTANCE   SPI4
#define BSP_SPI5_INSTANCE   SPI5

/* UART5 — PB8 RX, PB9 TX (пінами керує HAL_UART_MspInit, тут лише instance) */
#define BSP_UART5_INSTANCE  UART5

/* GPIO_Output — PA2, PA3, PE4. Перший подвоєно як BSP_LED1_*: спільний
 * Services/network/ethernet.c (blink-задача) очікує саме цю назву на
 * будь-якій платі — див. Platform/BSP/H750VB/config_pin.h. */
#define BSP_LED1_PORT   GPIOA
#define BSP_LED1_PIN    GPIO_PIN_2
#define BSP_OUT1_PORT   BSP_LED1_PORT
#define BSP_OUT1_PIN    BSP_LED1_PIN
#define BSP_OUT2_PORT   GPIOA
#define BSP_OUT2_PIN    GPIO_PIN_3
#define BSP_OUT3_PORT   GPIOE
#define BSP_OUT3_PIN    GPIO_PIN_4

/* EXTI rising edge — PE3, PC1, PD14, PA10. Перший подвоєно як
 * BSP_BUTTON1_* для симетрії з H750VB (нічого спільного це поки не
 * використовує, але узгоджено з тим, як main.c коментує PE3). */
#define BSP_BUTTON1_PORT GPIOE
#define BSP_BUTTON1_PIN  GPIO_PIN_3
#define BSP_EXTI1_PORT  BSP_BUTTON1_PORT
#define BSP_EXTI1_PIN   BSP_BUTTON1_PIN
#define BSP_EXTI2_PORT  GPIOC
#define BSP_EXTI2_PIN   GPIO_PIN_1
#define BSP_EXTI3_PORT  GPIOD
#define BSP_EXTI3_PIN   GPIO_PIN_14
#define BSP_EXTI4_PORT  GPIOA
#define BSP_EXTI4_PIN   GPIO_PIN_10

#define BSP_RFM66_CS_PORT    GPIOA
#define BSP_RFM66_CS_PIN     GPIO_PIN_4
#define BSP_RFM66_RESET_PORT GPIOC
#define BSP_RFM66_RESET_PIN  GPIO_PIN_4

#define BSP_RFM66_DIO0_PORT  GPIOE
#define BSP_RFM66_DIO0_PIN   GPIO_PIN_7
#define BSP_RFM66_DIO1_PORT  GPIOB
#define BSP_RFM66_DIO1_PIN   GPIO_PIN_4
#define BSP_RFM66_DIO2_PORT  GPIOE
#define BSP_RFM66_DIO2_PIN   GPIO_PIN_8
#define BSP_RFM66_DIO3_PORT  GPIOB
#define BSP_RFM66_DIO3_PIN   GPIO_PIN_0
#define BSP_RFM66_DIO4_PORT  GPIOE
#define BSP_RFM66_DIO4_PIN   GPIO_PIN_9
#define BSP_RFM66_DIO5_PORT  GPIOB
#define BSP_RFM66_DIO5_PIN   GPIO_PIN_6
