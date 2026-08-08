#pragma once

#include "logger_record.h"

#include <stddef.h>

int logger_format_record(char *buffer, size_t buffer_size, const logger_record_t *record);