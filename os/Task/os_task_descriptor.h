#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

typedef struct os_task_descriptor
{
    /* Identification */
    const char          *name;

    /* Task entry */
    TaskFunction_t       entry;
    void                *argument;

    /* Static allocation */
    StackType_t         *stack;
    uint32_t             stack_size;
    StaticTask_t        *tcb;

    /* Runtime */
    TaskHandle_t        *handle;

    /* Scheduling */
    UBaseType_t          priority;

    /* Future extensions */
    uint32_t             runlevel;
    uint32_t             flags;

} os_task_descriptor_t;