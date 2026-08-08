#pragma once

#include <stdint.h>

typedef enum
{
  DEBUG_PANIC_NONE = 0,
  DEBUG_PANIC_ASSERT,
  DEBUG_PANIC_MALLOC_FAILED,
  DEBUG_PANIC_STACK_OVERFLOW,
} debug_panic_reason_t;

typedef struct
{
  volatile debug_panic_reason_t reason;
  volatile const char *file;
  volatile uint32_t line;
  volatile const char *context;
  volatile void *handle;
} debug_panic_record_t;

extern volatile debug_panic_record_t g_debug_panic_record;

void debug_tools_init(void);
void debug_panic(debug_panic_reason_t reason, const char *file, uint32_t line, const char *context, void *handle);