#include "logger_format.h"

#include "main.h"

#include <stdio.h>

#define LOGGER_FW_CODE           "H750VB"
#define LOGGER_COLOR_DEBUG       "\x1b[36m"
#define LOGGER_COLOR_INFO        "\x1b[32m"
#define LOGGER_COLOR_WARNING     "\x1b[33m"
#define LOGGER_COLOR_ERROR       "\x1b[31m"
#define LOGGER_COLOR_CRITICAL    "\x1b[35;1m"
#define LOGGER_COLOR_RESET       "\x1b[0m"

static uint32_t logger_device_id;
static int logger_identity_initialized;

static void logger_identity_init(void)
{
    if (logger_identity_initialized)
    {
        return;
    }

    logger_device_id = *((const uint32_t *)(UID_BASE + 8U));
    logger_identity_initialized = 1;
}

static const char *logger_level_to_string(log_level_t level)
{
    switch (level)
    {
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_WARNING:
            return "WARN";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        case LOG_LEVEL_CRITICAL:
            return "CRIT";
        default:
            return "UNK";
    }
}

static const char *logger_level_to_color(log_level_t level)
{
    switch (level)
    {
        case LOG_LEVEL_DEBUG:
            return LOGGER_COLOR_DEBUG;
        case LOG_LEVEL_INFO:
            return LOGGER_COLOR_INFO;
        case LOG_LEVEL_WARNING:
            return LOGGER_COLOR_WARNING;
        case LOG_LEVEL_ERROR:
            return LOGGER_COLOR_ERROR;
        case LOG_LEVEL_CRITICAL:
            return LOGGER_COLOR_CRITICAL;
        default:
            return LOGGER_COLOR_RESET;
    }
}

static const char *logger_module_to_string(log_module_t module)
{
    switch (module)
    {
        case LOG_MODULE_APP:
            return "app";
        case LOG_MODULE_ETH:
            return "eth";
        case LOG_MODULE_RFM:
            return "rfm";
        case LOG_MODULE_SENSOR:
            return "sensor";
        case LOG_MODULE_STORAGE:
            return "storage";
        case LOG_MODULE_SYSTEM:
            return "system";
        default:
            return "unknown";
    }
}

static void logger_format_uptime(
    uint32_t uptime_ms,
    uint32_t *hours,
    uint32_t *minutes,
    uint32_t *seconds,
    uint32_t *milliseconds)
{
    *hours = uptime_ms / 3600000UL;
    uptime_ms %= 3600000UL;

    *minutes = uptime_ms / 60000UL;
    uptime_ms %= 60000UL;

    *seconds = uptime_ms / 1000UL;
    *milliseconds = uptime_ms % 1000UL;
}

int logger_format_record(char *buffer, size_t buffer_size, const logger_record_t *record)
{
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;
    int length;

    if ((buffer == NULL) || (buffer_size == 0U) || (record == NULL))
    {
        return -1;
    }

    logger_identity_init();

    logger_format_uptime(
        record->timestamp,
        &hours,
        &minutes,
        &seconds,
        &milliseconds
    );

    length = snprintf(
        buffer,
        buffer_size,
        "%s%s-%08lX> %03lu:%02lu:%02lu.%03lu %-5s %-7s %s%s\r\n",
        logger_level_to_color(record->level),
        LOGGER_FW_CODE,
        (unsigned long)logger_device_id,
        (unsigned long)hours,
        (unsigned long)minutes,
        (unsigned long)seconds,
        (unsigned long)milliseconds,
        logger_level_to_string(record->level),
        logger_module_to_string(record->module),
        record->message,
        LOGGER_COLOR_RESET
    );

    if (length < 0)
    {
        return -1;
    }

    if ((size_t)length >= buffer_size)
    {
        return (int)(buffer_size - 1U);
    }

    return length;
}