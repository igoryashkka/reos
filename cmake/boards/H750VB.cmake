# ==================================================================
# Board: H750VB (STM32H750, Cortex-M7)
#
# Визначає змінні, які споживає корінний CMakeLists.txt однаково для
# будь-якої плати: MX_Defines_Syms, MX_Include_Dirs, BOARD_Core_Src,
# BOARD_Drivers_Src, BSP_Include_Dir, FREERTOS_PORT_DIR. Порівняй з
# cmake/boards/F413.cmake — той самий набір змінних, інший вміст.
# ==================================================================

set(BOARD_DIR ${CMAKE_SOURCE_DIR}/Boards/H750VB)

set(MX_Defines_Syms
    USE_PWR_LDO_SUPPLY
    USE_HAL_DRIVER
    STM32H750xx
    $<$<CONFIG:Debug>:DEBUG>
)

set(FREERTOS_PORT_DIR ARM_CM4F)

set(MX_Include_Dirs
    ${BOARD_DIR}/Core/Inc
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${BOARD_DIR}/Drivers/CMSIS/RTOS2/Include
    ${BOARD_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${BOARD_DIR}/Drivers/CMSIS/Include
)

# Core/Src (+ startup) — компілюються прямо в таргет прошивки
set(BOARD_Core_Src
    ${BOARD_DIR}/Core/Src/main.c
    ${BOARD_DIR}/Core/Src/uart_stm32h750.c
    ${BOARD_DIR}/Core/Src/spi_stm32h750.c
    ${BOARD_DIR}/Core/Src/stm32h7xx_it.c
    ${BOARD_DIR}/Core/Src/stm32h7xx_hal_msp.c
    ${BOARD_DIR}/Core/Src/sysmem.c
    ${BOARD_DIR}/Core/Src/syscalls.c
    ${BOARD_DIR}/startup_stm32h750xx.s
)

# HAL/CMSIS — окрема OBJECT-бібліотека STM32_Drivers
set(BOARD_Drivers_Src
    ${BOARD_DIR}/Core/Src/system_stm32h7xx.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
    ${BOARD_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
)

set(BSP_Include_Dir ${CMAKE_SOURCE_DIR}/Platform/BSP/H750VB)
