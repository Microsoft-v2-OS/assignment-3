#include "memory_monitor.h"

#include <stdio.h>
#include <stdlib.h>

#define TEST_BYTES (16UL * 1024UL * 1024UL)
#define PAGE_BYTES 4096UL

static int snapshot(const char *stage, MemoryUsage *usage) {
    if (read_memory_usage(usage) != 0) {
        perror("read_memory_usage");
        return -1;
    }
    print_memory_usage(stage, usage);
    return 0;
}

int main(void) {
    MemoryUsage before, reserved, touched, released;
    MemoryDelta delta;

    if (snapshot("Before", &before) != 0) {
        return EXIT_FAILURE;
    }

    volatile unsigned char *buffer = malloc(TEST_BYTES);
    if (buffer == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    if (snapshot("After allocation", &reserved) != 0) {
        free((void *)buffer);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < TEST_BYTES; i += PAGE_BYTES) {
        buffer[i] = 1;
    }

    if (snapshot("After page touch", &touched) != 0) {
        free((void *)buffer);
        return EXIT_FAILURE;
    }

    if (calculate_memory_delta(&before, &reserved, &delta) != 0) {
        perror("calculate_memory_delta");
        free((void *)buffer);
        return EXIT_FAILURE;
    }
    print_memory_delta("Allocation delta", &delta);

    if (calculate_memory_delta(&reserved, &touched, &delta) != 0) {
        perror("calculate_memory_delta");
        free((void *)buffer);
        return EXIT_FAILURE;
    }
    print_memory_delta("Page-touch delta", &delta);

    free((void *)buffer);
    if (snapshot("After free", &released) != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
