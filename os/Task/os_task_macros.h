#pragma once

#include "os_task_descriptor.h"

#define OS_TASK_SECTION \
    __attribute__((used, section(".os.task.desc")))

#define OS_TASK_DEFINE(_name, _entry, _arg, _prio, _stack_size)     \
                                                                    \
    static StackType_t _name##_stack[_stack_size];                  \
                                                                    \
    static StaticTask_t _name##_tcb;                                \
                                                                    \
    static TaskHandle_t _name##_handle;                             \
                                                                    \
    const os_task_descriptor_t _name##_descriptor                   \
    OS_TASK_SECTION =                                               \
    {                                                               \
        .name       = #_name,                                       \
        .entry      = (_entry),                                     \
        .argument   = (_arg),                                       \
        .priority   = (_prio),                                      \
        .stack      = _name##_stack,                                \
        .stack_size = _stack_size,                                  \
        .tcb        = &_name##_tcb,                                 \
        .handle     = &_name##_handle,                              \
        .runlevel   = OS_RUNLEVEL_DEFAULT,                          \
        .flags      = 0                                             \
    }
