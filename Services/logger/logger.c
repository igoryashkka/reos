#include "os_task.h"

static void logger_task(void *arg)
{
    for (;;)
    {
        
    }
}

OS_TASK_DEFINE(
    logger,
    logger_task,
    NULL,
    OS_PRIORITY_HIGH,
    1024
);
