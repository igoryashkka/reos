#include "logger.h"
#include "logger_backend.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


static QueueHandle_t logger_queue;

static logger_record_t logger_queue_storage[32];

static StaticQueue_t logger_queue_control;

static uint32_t logger_get_timestamp(void)
{
    return (uint32_t)xTaskGetTickCount();
}


void logger_init(void)
{
    if (logger_queue != NULL)
    {
        return;
    }

    logger_queue = xQueueCreateStatic(
        32,
        sizeof(logger_record_t),
        (uint8_t *)logger_queue_storage,
        &logger_queue_control
    );

    logger_backend_init();
}

void logger_log(
    log_level_t level,
    log_module_t module,
    const char *format,
    ...
)
{
    if (logger_queue == NULL)
    {
        return;
    }

    logger_record_t record = {0};

    record.level = level;
    record.module = module;

    record.timestamp = logger_get_timestamp();

    va_list args;

    va_start(args, format);

    int length = vsnprintf(
        record.message,
        sizeof(record.message),
        format,
        args
    );

    va_end(args);

    if (length < 0)
    {
        return;
    }

    if (length >= sizeof(record.message))
    {
        length = sizeof(record.message) - 1;
    }

    record.length = length;

    xQueueSend(
        logger_queue,
        &record,
        0
    );
}

void logger_flush(void)
{
    logger_backend_flush();
}

void logger_process(void)
{
    logger_record_t record;

    if (logger_queue == NULL)
    {
        return;
    }

    if (xQueueReceive(logger_queue, &record, portMAX_DELAY) == pdTRUE)
    {
        logger_backend_write(&record);
    }
}