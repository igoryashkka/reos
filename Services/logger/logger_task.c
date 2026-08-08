#include "logger.h"
#include "os_task.h"

static void logger_task(void *arg)
{
    (void)arg;

    logger_init();
    LOG_INFO(LOG_MODULE_SYSTEM, "logger task online");

    for (;;)
    {
        logger_process();
    }
}

OS_TASK_DEFINE(
    logger,
    logger_task,
    NULL,
    OS_PRIORITY_HIGH,
    1024
);
