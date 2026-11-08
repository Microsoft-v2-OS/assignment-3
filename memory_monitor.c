
#include "memory_monitor.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parse a value such as "   12345 kB\n" and reject malformed input. */
static int parse_memory_kb(const char *text, unsigned long long *value) {
    if (text == NULL || value == NULL) {
        errno = EINVAL;
        return -1;
    }

    while (isspace((unsigned char)*text)) {
        ++text;
    }
    if (!isdigit((unsigned char)*text)) {
        errno = EIO;
        return -1;
    }

    errno = 0;
    char *end;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno == ERANGE) {
        return -1;
    }

    while (isspace((unsigned char)*end)) {
        ++end;
    }
    if (end[0] != 'k' || end[1] != 'B') {
        errno = EIO;
        return -1;
    }
    end += 2;

    while (isspace((unsigned char)*end)) {
        ++end;
    }
    if (*end != '\0') {
        errno = EIO;
        return -1;
    }

    *value = parsed;
    return 0;
}

int read_memory_usage(MemoryUsage *usage) {
    if (usage == NULL) {
        errno = EINVAL;
        return -1;
    }

    FILE *file = fopen("/proc/self/status", "r");
    if (file == NULL) {
        return -1;
    }

    MemoryUsage result = {0, 0};
    int has_vm_size = 0;
    int has_vm_rss = 0;
    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        int parse_result = 0;

        if (strncmp(line, "VmSize:", 7) == 0) {
            parse_result = parse_memory_kb(line + 7, &result.vm_size_kb);
            has_vm_size = (parse_result == 0);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            parse_result = parse_memory_kb(line + 6, &result.vm_rss_kb);
            has_vm_rss = (parse_result == 0);
        }

        if (parse_result != 0) {
            int saved_errno = errno;
            (void)fclose(file);
            errno = saved_errno;
            return -1;
        }

        if (has_vm_size && has_vm_rss) {
            break;
        }
    }

    if (ferror(file)) {
        int saved_errno = errno != 0 ? errno : EIO;
        (void)fclose(file);
        errno = saved_errno;
        return -1;
    }

    if (fclose(file) != 0) {
        return -1;
    }

    if (!has_vm_size || !has_vm_rss) {
        errno = EIO;
        return -1;
    }

    *usage = result;
    return 0;
}

void print_memory_usage(const char *label, const MemoryUsage *usage) {
    if (usage == NULL) {
        return;
    }

    const char *stage = label != NULL ? label : "Current";
    printf("VmSize %s: %llu kB | VmRSS %s: %llu kB\n",
           stage, usage->vm_size_kb, stage, usage->vm_rss_kb);
}

int calculate_memory_delta(const MemoryUsage *before,
                           const MemoryUsage *after,
                           MemoryDelta *delta) {
    if (before == NULL || after == NULL || delta == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Values must fit in signed long long for a safe subtraction. */
    if (before->vm_size_kb > LLONG_MAX || after->vm_size_kb > LLONG_MAX ||
        before->vm_rss_kb > LLONG_MAX || after->vm_rss_kb > LLONG_MAX) {
        errno = ERANGE;
        return -1;
    }

    MemoryDelta result;
    result.vm_size_change_kb = (long long)after->vm_size_kb -
                               (long long)before->vm_size_kb;
    result.vm_rss_change_kb = (long long)after->vm_rss_kb -
                              (long long)before->vm_rss_kb;

    *delta = result;
    return 0;
}

void print_memory_delta(const char *label, const MemoryDelta *delta) {
    if (delta == NULL) {
        return;
    }

    printf("%s: Delta VmSize = %+lld kB | Delta VmRSS = %+lld kB\n",
           label != NULL ? label : "Change",
           delta->vm_size_change_kb, delta->vm_rss_change_kb);
}
