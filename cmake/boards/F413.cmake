# ==================================================================
# Board: F413 (STM32F413, Cortex-M4F)
#
# Той самий набір змінних, що й cmake/boards/H750VB.cmake, інший вміст.
# FreeRTOS (Middlewares/) свідомо СПІЛЬНИЙ з H750VB (root CMakeLists.txt) —
# завантажений з F413 архів мав іншу версію ядра (V10.3.1 проти
# V10.6.2), тож його Middlewares/ не копіювались узагалі, аби не
# тримати дві версії FreeRTOS в одному репо.
# ==================================================================

set(BOARD_DIR ${CMAKE_SOURCE_DIR}/Boards/F413)

set(MX_Defines_Syms
    USE_HAL_DRIVER
    STM32F413xx
    $<$<CONFIG:Debug>:DEBUG>
)

set(FREERTOS_PORT_DIR ARM_CM4F)

set(MX_Include_Dirs
    ${BOARD_DIR}/Core/Inc
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${BOARD_DIR}/Drivers/CMSIS/RTOS2/Include
    ${BOARD_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${BOARD_DIR}/Drivers/CMSIS/Include
)

# Core/Src (+ startup) — компілюються прямо в таргет прошивки
set(BOARD_Core_Src
    ${BOARD_DIR}/Core/Src/main.c
    ${BOARD_DIR}/Core/Src/uart_stm32f413.c
    ${BOARD_DIR}/Core/Src/spi_stm32f413.c
    ${BOARD_DIR}/Core/Src/stm32f4xx_it.c
    ${BOARD_DIR}/Core/Src/stm32f4xx_hal_msp.c
    ${BOARD_DIR}/Core/Src/sysmem.c
    ${BOARD_DIR}/Core/Src/syscalls.c
    ${BOARD_DIR}/startup_stm32f413xx.s
)

# HAL/CMSIS — окрема OBJECT-бібліотека STM32_Drivers. Список під те, що
# реально задіяне в MX_GPIO/SPI1-5/UART5_Init + HAL_*_MspInit (SPI, UART,
# GPIO, EXTI, RCC, FLASH, PWR, DMA — MSP лінкує DMA-заголовки типів,
# навіть якщо жоден handle DMA не використовує).
set(BOARD_Drivers_Src
    ${BOARD_DIR}/Core/Src/system_stm32f4xx.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
    ${BOARD_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
)

set(BSP_Include_Dir ${CMAKE_SOURCE_DIR}/Platform/BSP/F413)
