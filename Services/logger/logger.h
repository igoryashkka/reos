#pragma once

#include <stdint.h>
#include <stddef.h>

#define LOG_DEBUG(module, ...) \
    logger_log(LOG_LEVEL_DEBUG, module, __VA_ARGS__)

#define LOG_INFO(module, ...) \
    logger_log(LOG_LEVEL_INFO, module, __VA_ARGS__)

#define LOG_WARNING(module, ...) \
    logger_log(LOG_LEVEL_WARNING, module, __VA_ARGS__)

#define LOG_ERROR(module, ...) \
    logger_log(LOG_LEVEL_ERROR, module, __VA_ARGS__)

#define LOG_CRITICAL(module, ...) \
    logger_log(LOG_LEVEL_CRITICAL, module, __VA_ARGS__)

typedef enum
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL

} log_level_t;


typedef enum
{
    LOG_MODULE_APP = 0,
    LOG_MODULE_ETH,
    LOG_MODULE_RFM,
    LOG_MODULE_SENSOR,
    LOG_MODULE_STORAGE,
    LOG_MODULE_SYSTEM,

    LOG_MODULE_COUNT

} log_module_t;


/**
 * Initialize logger subsystem.
 */
void logger_init(void);


/**
 * Write a log message.
 */
void logger_log(
    log_level_t level,
    log_module_t module,
    const char *format,
    ...
);


/**
 * Flush pending logs.
 */
void logger_flush(void);

void logger_process(void);