#pragma once

#include "logger_record.h"

typedef struct logger_backend
{
    const char *name;

    int (*init)(void);

    int (*write)(
        const logger_record_t *record
    );

    int (*flush)(void);

} logger_backend_t;


void logger_backend_init(void);

void logger_backend_write(
    const logger_record_t *record
);

void logger_backend_flush(void);