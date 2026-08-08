#include "debug_tools.h"

#include "main.h"

volatile debug_panic_record_t g_debug_panic_record = {
    .reason = DEBUG_PANIC_NONE,
    .file = 0,
    .line = 0,
    .context = 0,
    .handle = 0,
};

static void debug_break_if_attached(void)
{
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U)
    {
        __BKPT(0);
    }
}

void debug_tools_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void debug_panic(debug_panic_reason_t reason, const char *file, uint32_t line, const char *context, void *handle)
{
    __disable_irq();

    g_debug_panic_record.reason = reason;
    g_debug_panic_record.file = file;
    g_debug_panic_record.line = line;
    g_debug_panic_record.context = context;
    g_debug_panic_record.handle = handle;

    debug_break_if_attached();

    for (;;)
    {
    }
}