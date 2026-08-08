#include "os_task.h"
#include "os_task_descriptor.h"

extern const os_task_descriptor_t __os_task_desc_start;
extern const os_task_descriptor_t __os_task_desc_end;


void os_task_init_all(void)
{
    const os_task_descriptor_t *task;

    for (task = &__os_task_desc_start;
         task < &__os_task_desc_end;
         task++)
    {
        *(task->handle) = xTaskCreateStatic(
            task->entry,
            task->name,
            task->stack_size,
            task->argument,
            task->priority,
            task->stack,
            task->tcb
        );
    }
}