#ifndef LOGGER_STD_H
#define LOGGER_STD_H

#include <stdio.h>

#include "memory_monitor.h"

typedef struct {
    FILE *file;
    char filename[256];
} StdLogger;


int std_logger_open(StdLogger *logger, const char *group_id,
                    const char *method);

int std_logger_printf(StdLogger *logger, const char *format, ...);

int std_logger_log_usage(StdLogger *logger, const char *stage,
                         const MemoryUsage *usage);

int std_logger_log_delta(StdLogger *logger, const MemoryDelta *delta);

int std_logger_close(StdLogger *logger);

#endif
