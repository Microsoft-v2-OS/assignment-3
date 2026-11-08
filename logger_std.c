#include "logger_std.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static int valid_group_id(const char *group_id) {
    if (group_id == NULL || *group_id == '\0') {
        return 0;
    }

    for (const unsigned char *p = (const unsigned char *)group_id;
         *p != '\0'; ++p) {
        if (!isalnum(*p) && *p != '_' && *p != '-') {
            return 0;
        }
    }
    return 1;
}

int std_logger_open(StdLogger *logger, const char *group_id,
                    const char *method) {
    if (logger == NULL || !valid_group_id(group_id) || method == NULL ||
        (strcmp(method, "malloc") != 0 && strcmp(method, "mmap") != 0)) {
        errno = EINVAL;
        return -1;
    }

    /* Logger harus diinisialisasi dengan {0} sebelum pertama kali dipakai. */
    if (logger->file != NULL) {
        errno = EBUSY;
        return -1;
    }
    logger->filename[0] = '\0';

    struct timespec now;
    if (timespec_get(&now, TIME_UTC) != TIME_UTC) {
        errno = EIO;
        return -1;
    }

    struct tm *local = localtime(&now.tv_sec);
    char timestamp[32];
    if (local == NULL ||
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", local) == 0) {
        errno = EIO;
        return -1;
    }

    int n = snprintf(logger->filename, sizeof(logger->filename),
                     "%s-%s_std-%s_%09ld.txt",
                     group_id, method, timestamp, now.tv_nsec);
    if (n < 0 || (size_t)n >= sizeof(logger->filename)) {
        logger->filename[0] = '\0';
        errno = ENAMETOOLONG;
        return -1;
    }

    /* Mode wx tidak menimpa log yang sudah ada. */
    logger->file = fopen(logger->filename, "wx");
    if (logger->file == NULL) {
        logger->filename[0] = '\0';
        return -1;
    }
    return 0;
}

int std_logger_printf(StdLogger *logger, const char *format, ...) {
    if (logger == NULL || logger->file == NULL || format == NULL) {
        errno = EINVAL;
        return -1;
    }

    va_list args;
    va_list copy;
    va_start(args, format);
    va_copy(copy, args);

    int written_file = vfprintf(logger->file, format, args);
    int written_screen = vfprintf(stdout, format, copy);

    va_end(copy);
    va_end(args);

    if (written_file < 0 || written_screen < 0) {
        return -1;
    }
    return written_file;
}

int std_logger_log_usage(StdLogger *logger, const char *stage,
                         const MemoryUsage *usage) {
    if (logger == NULL || logger->file == NULL ||
        stage == NULL || usage == NULL) {
        errno = EINVAL;
        return -1;
    }

    int written_file = fprintf(logger->file,
                               "VmSize %s: %llu kB | VmRSS %s: %llu kB\n",
                               stage, usage->vm_size_kb,
                               stage, usage->vm_rss_kb);
    int written_screen = printf("VmSize %s: %llu kB | VmRSS %s: %llu kB\n",
                                stage, usage->vm_size_kb,
                                stage, usage->vm_rss_kb);
    return (written_file < 0 || written_screen < 0) ? -1 : 0;
}

int std_logger_log_delta(StdLogger *logger, const MemoryDelta *delta) {
    if (logger == NULL || logger->file == NULL || delta == NULL) {
        errno = EINVAL;
        return -1;
    }

    int written_file = fprintf(logger->file,
                               "Delta VmSize: %+lld kB | Delta VmRSS: %+lld kB\n",
                               delta->vm_size_change_kb,
                               delta->vm_rss_change_kb);
    int written_screen = printf("Delta VmSize: %+lld kB | Delta VmRSS: %+lld kB\n",
                                delta->vm_size_change_kb,
                                delta->vm_rss_change_kb);
    return (written_file < 0 || written_screen < 0) ? -1 : 0;
}

int std_logger_close(StdLogger *logger) {
    if (logger == NULL || logger->file == NULL) {
        errno = EINVAL;
        return -1;
    }

    FILE *file = logger->file;
    logger->file = NULL;
    return fclose(file) == 0 ? 0 : -1;
}
