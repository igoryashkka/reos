cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME H750VB)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# BOARD обирає ціль (H750VB | F413) — MCU/лінкер вже вибрані на рівні
# toolchain-файлу (cmake/gcc-arm-none-eabi.cmake, та сама змінна), тут
# лише завантажуються джерела/include-шляхи/дефайни конкретної плати.
# App/, Services/, os/ (у т.ч. os/freertos.c — реалізація os_main())
# лишаються однаковими для будь-якої плати — вони нижче в цьому файлі,
# а не в cmake/boards/*.cmake.
if(NOT DEFINED BOARD)
    set(BOARD "H750VB")
endif()
set(BOARD ${BOARD} CACHE STRING "Target board: H750VB | F413")
set_property(CACHE BOARD PROPERTY STRINGS H750VB F413)

set(BOARD_VALID_VALUES H750VB F413)
if(NOT BOARD IN_LIST BOARD_VALID_VALUES)
    message(FATAL_ERROR "BOARD='${BOARD}' is not valid. Choose exactly one of: ${BOARD_VALID_VALUES}")
endif()

include(cmake/boards/${BOARD}.cmake)

# rp_* wireless network model selection, see Services/wireless/rp_config.cmake
include(Services/wireless/rp_config.cmake)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Interface library for the active board's include dirs and symbols,
# plus everything shared (BSP contract headers, os/, Services/, FreeRTOS
# API headers). Mirrors what used to be cmake/stm32cubemx/CMakeLists.txt,
# now board-parameterized instead of hardcoded to H750VB.
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE
    ${MX_Include_Dirs}
    ${BSP_Include_Dir}
    ${CMAKE_SOURCE_DIR}/Platform/BSP
    ${CMAKE_SOURCE_DIR}/os
    ${CMAKE_SOURCE_DIR}/os/Task
    ${CMAKE_SOURCE_DIR}/Services/logger
    ${CMAKE_SOURCE_DIR}/Services/wireless
    ${CMAKE_SOURCE_DIR}/Services/wireless/massges
    ${CMAKE_SOURCE_DIR}/Services/wireless/port
    ${CMAKE_SOURCE_DIR}/Drivers/rfm66
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/include
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/${FREERTOS_PORT_DIR}
)
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Board-specific HAL/CMSIS static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${BOARD_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)

# FreeRTOS kernel — SAME for every board (Middlewares/ is not duplicated
# per board; see cmake/boards/F413.cmake's comment on why F413's own
# downloaded copy was discarded instead of vendored alongside it).
add_library(FreeRTOS OBJECT)
target_sources(FreeRTOS PRIVATE
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/Common/mpu_wrappers_v2.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/croutine.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/list.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/queue.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/tasks.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/timers.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/${FREERTOS_PORT_DIR}/port.c
)
target_link_libraries(FreeRTOS PUBLIC stm32cubemx)

# Board Core/Src + startup asm (main.c, interrupts, MSP, syscalls, ...)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${BOARD_Core_Src})

# Shared application/service sources — identical regardless of BOARD.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/App/application.c
    ${CMAKE_SOURCE_DIR}/os/freertos.c
    ${CMAKE_SOURCE_DIR}/os/debug_tools.c
    ${CMAKE_SOURCE_DIR}/os/Task/os_task.c
    ${CMAKE_SOURCE_DIR}/Services/network/ethernet.c
    ${CMAKE_SOURCE_DIR}/Services/logger/logger.c
    ${CMAKE_SOURCE_DIR}/Services/logger/logger_backend.c
    ${CMAKE_SOURCE_DIR}/Services/logger/logger_format.c
    ${CMAKE_SOURCE_DIR}/Services/logger/logger_task.c
    ${CMAKE_SOURCE_DIR}/Services/logger/logger_record.c
    ${CMAKE_SOURCE_DIR}/Services/logger/backends/logger_uart.c
    ${CMAKE_SOURCE_DIR}/Services/wireless/rp_proto.c
    ${CMAKE_SOURCE_DIR}/Services/wireless/rp_msg.c
    ${CMAKE_SOURCE_DIR}/Services/wireless/rp_hw_if.c
    ${CMAKE_SOURCE_DIR}/Services/wireless/backends/rp_hw_uart.c
    ${CMAKE_SOURCE_DIR}/Services/wireless/backends/rp_hw_radio.c
    ${CMAKE_SOURCE_DIR}/Drivers/rfm66/rfm66.c
)

# rp_* wireless network model (RP_NETWORK_MODEL/RP_IOT_ROLE), see
# Services/wireless/rp_config.cmake. Host unit tests are a separate,
# standalone native CMake project (Services/wireless/tests/), not a
# value of RP_NETWORK_MODEL — this preset always builds real firmware.
if(RP_NETWORK_MODEL STREQUAL "IOT")
    rp_configure_network(${CMAKE_PROJECT_NAME} IOT freertos ${RP_IOT_ROLE})
else()
    rp_configure_network(${CMAKE_PROJECT_NAME} ${RP_NETWORK_MODEL} freertos)
endif()

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    FreeRTOS

    # Add user defined libraries
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    BYPRODUCTS ${CMAKE_PROJECT_NAME}.hex ${CMAKE_PROJECT_NAME}.bin
    COMMENT "Generating HEX and BIN artifacts"
)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
