/* Uji mandiri untuk subbab 5.6.1, bukan bagian modul program utama. */
#include "logger_std.h"
#include "memory_monitor.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALLOCATION_BYTES (4u * 1024u * 1024u)
#define PAGE_BYTES 4096u

int main(void) {
    StdLogger logger = {0};
    MemoryUsage before, after;
    MemoryDelta delta;
    unsigned char *buffer = NULL;
    int status = EXIT_FAILURE;

    if (std_logger_open(&logger, "04", "malloc") != 0) {
        perror("std_logger_open");
        return EXIT_FAILURE;
    }

    if (std_logger_printf(&logger, "=== STANDARD C LOGGER TEST ===\n") < 0 ||
        std_logger_printf(&logger, "Allocation: %u bytes\n", ALLOCATION_BYTES) < 0) {
        perror("std_logger_printf");
        goto cleanup;
    }

    if (read_memory_usage(&before) != 0 ||
        std_logger_log_usage(&logger, "Before", &before) != 0) {
        perror("read/log before");
        goto cleanup;
    }

    buffer = malloc(ALLOCATION_BYTES);
    if (buffer == NULL) {
        perror("malloc");
        goto cleanup;
    }

    for (size_t i = 0; i < ALLOCATION_BYTES; i += PAGE_BYTES) {
        buffer[i] = (unsigned char)(i / PAGE_BYTES);
    }
    buffer[ALLOCATION_BYTES - 1] = 1;

    if (std_logger_printf(&logger, "Buffer address: %p\n", (void *)buffer) < 0 ||
        read_memory_usage(&after) != 0 ||
        std_logger_log_usage(&logger, "After page touch", &after) != 0 ||
        calculate_memory_delta(&before, &after, &delta) != 0 ||
        std_logger_log_delta(&logger, &delta) != 0) {
        perror("read/log after");
        goto cleanup;
    }

    status = EXIT_SUCCESS;

cleanup:
    free(buffer);
    if (std_logger_close(&logger) != 0) {
        perror("std_logger_close");
        status = EXIT_FAILURE;
    }
    if (status == EXIT_SUCCESS) {
        printf("Log saved: %s\n", logger.filename);
    }
    return status;
}
