/*
 * Varian 4 - Demand Paging & Alokasi Lambat (Padjri).
 * mmap memesan ruang alamat virtual; halaman fisik baru diperlukan saat
 * ditulis. Satu byte ditulis pada setiap halaman, bukan memset seluruh blok.
 * volatile menjaga agar compiler tidak membuang store pada percobaan ini.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "demand_paging.h"
#include "memory_monitor.h"
#include "logger_std.h"
#include "logger_sys.h"

#define PAGE_BYTES ((size_t)4096)
#define MIB_BYTES ((size_t)1024 * 1024)
#define MAPPING_BYTES ((size_t)1024 * MIB_BYTES)

/* Kontrak logger mengikuti malloc_handler.c. Semua data eksperimen yang
 * tercetak dikirim ke kedua logger; prompt/menu tetap milik pemanggil.
 */
static void emit(const char *format, ...)
{
    char line[512];
    va_list args;

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    fputs(line, stdout);
    logger_std_write(line);
    logger_sys_write(line);
}

static int read_metrics(long *vmsize, long *vmrss)
{
    *vmsize = mem_get_vmsize_kb();
    *vmrss = mem_get_vmrss_kb();
    if (*vmsize < 0 || *vmrss < 0)
    {
        emit("Gagal membaca VmSize/VmRSS dari /proc/self/status.\n");
        return -1;
    }
    return 0;
}

void run_demand_paging(void)
{
    static const size_t stages_mib[] = {0, 5, 10, 25, 50, 75, 100};
    void *mapping = MAP_FAILED;
    volatile unsigned char *data;
    size_t touched_bytes = 0;
    size_t stage;
    long size_before, rss_before;
    long size_now, rss_now, rss_previous, rss_untouched = 0;
    long page_size;
    int saved_errno;

    /* "mmap" menjaga kompatibilitas penamaan log soal dan logger Adam/Akmal.
     * Judul di dalam log membedakan eksperimen ini dari menu mmap biasa.
     */
    if (logger_std_open("mmap") != 0)
    {
        fprintf(stderr, "Gagal membuka log Standard C Library.\n");
        return;
    }
    if (logger_sys_open("mmap") != 0)
    {
        fprintf(stderr, "Gagal membuka log System Call.\n");
        logger_std_close();
        return;
    }

    emit("--- Varian 4: Demand Paging ---\n");
    page_size = sysconf(_SC_PAGESIZE);
    if (page_size != (long)PAGE_BYTES)
    {
        emit("Demo memerlukan halaman 4096 byte; sysconf melaporkan %ld.\n",
             page_size);
        goto cleanup;
    }
    emit("Alokasi virtual: 1024 MiB (1 GiB); penulisan per 4096 byte.\n");
    emit("Tahap sentuh: 0, 5, 10, 25, 50, 75, 100 MiB.\n");
    if (read_metrics(&size_before, &rss_before) != 0)
        goto cleanup;
    emit("Sebelum mmap: VmSize = %ld KB | VmRSS = %ld KB\n",
         size_before, rss_before);

    /* Tanpa MAP_POPULATE: jangan fault-in seluruh 1 GiB saat mmap. */
    mapping = mmap(NULL, MAPPING_BYTES, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED)
    {
        saved_errno = errno;
        emit("mmap gagal: %s\n", strerror(saved_errno));
        goto cleanup;
    }
    emit("Starting memory address: %p\n", mapping);

    /* THP dapat mengalokasikan 2 MiB sekaligus. Nonaktifkan hanya pada
     * pemetaan eksperimen agar langkah 4 KiB lebih mudah diamati.
     */
#ifdef MADV_NOHUGEPAGE
    if (madvise(mapping, MAPPING_BYTES, MADV_NOHUGEPAGE) != 0)
    {
        saved_errno = errno;
        emit("Peringatan: MADV_NOHUGEPAGE gagal: %s; THP dapat memengaruhi RSS.\n",
             strerror(saved_errno));
    }
#else
    emit("MADV_NOHUGEPAGE tidak tersedia; THP dapat memengaruhi RSS.\n");
#endif
    emit("Delta RSS dihitung terhadap baris sebelumnya; baris 0 bernilai +0.\n");
    emit("Disentuh (MiB)  Halaman  VmSize (KB)  VmRSS (KB)  delta RSS (KB)\n");
    data = (volatile unsigned char *)mapping;
    rss_previous = 0;

    for (stage = 0; stage < sizeof(stages_mib) / sizeof(stages_mib[0]); stage++)
    {
        size_t target_bytes = stages_mib[stage] * MIB_BYTES;

        /* Setiap halaman hanya disentuh sekali, melanjutkan tahap sebelumnya.
         * 5 MiB / 4096 = 1280 halaman; 100 MiB = 25600 halaman.
         */
        while (touched_bytes < target_bytes)
        {
            data[touched_bytes] = 1;
            touched_bytes += PAGE_BYTES;
        }
        if (read_metrics(&size_now, &rss_now) != 0)
            goto cleanup;
        if (stage == 0)
        {
            rss_untouched = rss_now;
            rss_previous = rss_now;
            emit("Sesudah mmap, belum disentuh: delta VmSize = %+ld KB | "
                 "delta VmRSS = %+ld KB\n",
                 size_now - size_before, rss_now - rss_before);
        }
        emit("%14zu  %7zu  %11ld  %10ld  %+14ld\n",
             stages_mib[stage], touched_bytes / PAGE_BYTES,
             size_now, rss_now, rss_now - rss_previous);
        rss_previous = rss_now;
    }
    emit("Total delta VmRSS sejak 0 MiB: %+ld KB\n", rss_now - rss_untouched);

cleanup:
    if (mapping != MAP_FAILED)
    {
        if (munmap(mapping, MAPPING_BYTES) != 0)
        {
            saved_errno = errno;
            emit("munmap gagal: %s\n", strerror(saved_errno));
        }
        else if (read_metrics(&size_now, &rss_now) == 0)
        {
            emit("Sesudah munmap: VmSize = %ld KB | VmRSS = %ld KB\n",
                 size_now, rss_now);
        }
    }
    logger_sys_close();
    logger_std_close();
}
