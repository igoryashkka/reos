#pragma once

#include "logger.h"

#include <stdint.h>

#define LOGGER_MESSAGE_MAX 128

typedef struct
{
    uint32_t timestamp;
    log_level_t level;
    log_module_t module;
    const char *file;
    uint32_t line;
    uint16_t length;
    char message[LOGGER_MESSAGE_MAX];
} logger_record_t;