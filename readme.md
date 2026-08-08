cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --clean-first

Project
│
├── App/
│   ├── application/
│   ├── command/
│   ├── config/
│   └── state_machine/
│
├── Services/
│   ├── logger/
│   ├── network/
│   ├── radio/
│   ├── sensor/
│   ├── storage/
│   └── telemetry/
│
├── Drivers/
│   ├── adc/
│   ├── eth/
│   ├── flash/
│   ├── gpio/
│   ├── i2c/
│   ├── rfm/
│   ├── spi/
│   └── uart/
│
├── Platform/
│   ├── BSP/
│   ├── HAL/
│   ├── Startup/
│   └── Linker/
│
├── Kernel/
│   └── FreeRTOS/
│       ├── Source/
│       ├── portable/
│       └── FreeRTOSConfig.h
│
├── OS/
│   ├── Core/
│   ├── Task/
│   ├── Scheduler/
│   ├── Device/
│   ├── Init/
│   ├── Sync/
│   ├── Memory/
│   ├── Statistics/
│   ├── CLI/
│   └── Include/
│
├── Common/
│   ├── crc/
│   ├── event/
│   ├── queue/
│   ├── ringbuffer/
│   └── utils/
│
└── Tests/