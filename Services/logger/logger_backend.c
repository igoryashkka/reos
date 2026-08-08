#include "logger_backend.h"

extern const logger_backend_t logger_uart_backend;


static const logger_backend_t *backends[] =
{
    &logger_uart_backend
};


#define BACKEND_COUNT \
    (sizeof(backends) / sizeof(backends[0]))


void logger_backend_init(void)
{
    for (size_t i = 0; i < BACKEND_COUNT; i++)
    {
        if (backends[i]->init)
        {
            backends[i]->init();
        }
    }
}

void logger_backend_write(
    const logger_record_t *record
)
{
    for (size_t i = 0; i < BACKEND_COUNT; i++)
    {
        if (backends[i]->write)
        {
            backends[i]->write(record);
        }
    }
}

void logger_backend_flush(void)
{
    for (size_t i = 0; i < BACKEND_COUNT; i++)
    {
        if (backends[i]->flush)
        {
            backends[i]->flush();
        }
    }
}