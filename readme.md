# H750VB

STM32 firmware, FreeRTOS-based, multi-board (STM32H750VB, STM32F413) with a
shared App/OS/services layer and a `rp_*` wireless stack (P2P mesh or IoT
leaf/router/gateway, selected at compile time).

## Build

Board is picked via CMake presets — each preset has its own `build/<preset>/`
directory, so switching boards never needs a clean/reconfigure.

**H750VB** (default):

```bash
cmake --preset Debug --clean-first    
cmake --build --preset Debug --clean-first    
```

**F413**:

```bash
cmake --preset Debug-F413 --clean-first    
cmake --build --preset Debug-F413 --clean-first    
```

`Release` / `Release-F413` presets exist the same way. Under the hood the
board is a CMake cache variable (`BOARD=H750VB|F413`), set automatically by
the preset — see [cmake/gcc-arm-none-eabi.cmake](cmake/gcc-arm-none-eabi.cmake)
and [cmake/boards/](cmake/boards/).

The wireless stack has its own selection, independent of `BOARD`:
`RP_NETWORK_MODEL=P2P|IOT`, and `RP_IOT_ROLE=LEAF|ROUTER|GATEWAY` when IOT —
see [Services/wireless/rp_config.cmake](Services/wireless/rp_config.cmake).

```bash
cmake --preset Debug -DRP_NETWORK_MODEL=IOT -DRP_IOT_ROLE=GATEWAY
```

Host-side unit tests for the wireless stack build and run natively (no
hardware, no ARM toolchain) as their own standalone CMake project:

```bash
cmake -S Services/wireless/tests -B build/wireless-tests
cmake --build build/wireless-tests
ctest --test-dir build/wireless-tests
```

## Structure

```text
H750VB/
├── App/                     application.c — board/network-model agnostic
├── Services/
│   ├── logger/              log service + pluggable backends (UART, ...)
│   ├── network/              placeholder Ethernet task
│   └── wireless/             rp_* protocol stack
│       ├── rp_proto.*        frame format, CRC, RX parser
│       ├── rp_msg.*          message codec (core + TLV)
│       ├── rp_hw_if.*        + backends/    physical transport vtable (UART/radio)
│       ├── massges/          rp_link_t session + per-message builders
│       ├── common/           shared common/: frameq, duty-cycle accounting
│       ├── network/          rp_network.h — the one API App/ sees
│       │   ├── iot/          leaf/router/gateway roles + common/ (nodetab, mailbox)
│       │   └── p2p/          mesh routing: neighbor/route tables, forwarding
│       ├── port/             rp_port.h time abstraction (FreeRTOS / host)
│       ├── tests/            native host unit tests (see Build, above)
│       └── rp_config.cmake   RP_NETWORK_MODEL / RP_IOT_ROLE build selection
├── os/                       shared OS layer
│   ├── freertos.c            os_main() — kernel init + start, same on every board
│   ├── debug_tools.*         panic/fault recording, chip-agnostic
│   └── Task/                 OS_TASK_DEFINE task registration
├── Platform/BSP/             board pin/peripheral constants
│   ├── uart.h                 shared driver contract (uart_init/write/flush)
│   ├── H750VB/config_pin.h    BSP_LED1_*, BSP_LOG_UART_*, ...
│   └── F413/config_pin.h      BSP_SPI*_INSTANCE, BSP_UART5_INSTANCE, ...
├── Boards/                   everything genuinely chip-specific, one dir per board
│   ├── H750VB/                Core/, Drivers/, linker script, startup .s, .ioc
│   └── F413/                  same shape, STM32F4xx HAL/CMSIS instead
├── Middlewares/Third_Party/FreeRTOS/   shared kernel (one copy, all boards)
├── cmake/
│   ├── gcc-arm-none-eabi.cmake   toolchain, parameterized by BOARD
│   └── boards/                   per-board source/include lists (H750VB.cmake, F413.cmake)
├── CMakeLists.txt
└── CMakePresets.json
```
