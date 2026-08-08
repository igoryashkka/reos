#pragma once

#include "FreeRTOS.h"
#include "task.h"

#define OS_PRIORITY_LOW      1
#define OS_PRIORITY_NORMAL   2
#define OS_PRIORITY_HIGH     3

#define OS_RUNLEVEL_DEFAULT  0U

#include "os_task_macros.h"

void os_task_init_all(void);