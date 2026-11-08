/*
 * Menu 2, alokasi memori menggunakan mmap() dan munmap(). (Adam)
 *
 * Fungsi run_mmap_munmap() dipanggil saat user memilih opsi 2 di menu utama.
 * Alur kerjanya dibuat sejajar dengan malloc_handler.c milik Gagah supaya
 * hasil perbandingan antara malloc() dan mmap() bisa diamati secara langsung:
 * 1. Buka kedua file log (versi Standard C Library dan versi System Call).
 * 2. Baca dan tampilkan VmSize serta VmRSS sebelum alokasi.
 * 3. Minta input jumlah integer dari user.
 * 4. Alokasikan memori menggunakan system call mmap() anonymous.
 * 5. Isi array dari N turun ke 1, lalu tampilkan alamat awal dan 10 nilai pertama.
 * 6. Baca dan tampilkan VmSize serta VmRSS sesudah alokasi dan pengisian.
 * 7. Lepas memori menggunakan system call munmap() dan cek metrik kembali.
 *
 * Perbedaan utama mmap() dibanding malloc():
 * - malloc() adalah fungsi C Standard Library (user-space allocator) yang
 *   mengelola heap (via brk) untuk ukuran kecil, dan baru memakai mmap untuk
 *   ukuran besar (>= M_MMAP_THRESHOLD, sekitar 128 KB).
 * - mmap() adalah system call langsung ke kernel untuk memetakan halaman memori
 *   virtual baru di ruang alamat proses (selalu sejajar dengan ukuran halaman /
 *   page-aligned, yaitu kelipatan 4096 byte).
 * - Saat mmap() dipanggil, VmSize naik, tetapi halaman fisik di RAM (VmRSS)
 *   baru dialokasikan saat array benar-benar ditulis (demand paging).
 * - Saat munmap() dipanggil, memori langsung dikembalikan ke kernel sehingga
 *   VmSize dan VmRSS langsung turun kembali, bahkan untuk ukuran alokasi kecil
 *   sekalipun (berbeda dengan free() pada heap yang sering menahan memori).
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#if defined(__has_include)
#if __has_include(<sys/mman.h>)
#include <sys/mman.h>
#else
#include <sys/types.h>
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
#endif
#else
#include <sys/mman.h>
#endif

#include "mmap_handler.h"
#include "memory_monitor.h"
#include "logger_std.h"
#include "logger_sys.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0x02
#endif
#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#define SHOW_COUNT 10
#define LINE_BUF_SIZE 256

#ifndef GROUP_PREFIX
#define GROUP_PREFIX "04"
#endif

/* Instance logger Standard C Library milik modul mmap */
static StdLogger std_logger = {0};

/* Menulis langsung ke berkas log std tanpa menduplikasi output terminal */
static void write_std_log(const char *text)
{
    if (std_logger.file != NULL && text != NULL)
    {
        fputs(text, std_logger.file);
        fflush(std_logger.file);
    }
}

/* Mencetak output ke layar sekaligus menulis ke kedua berkas log */
static void emit(const char *fmt, ...)
{
    char line[LINE_BUF_SIZE];
    va_list args;

    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    fputs(line, stdout);
    write_std_log(line);
    logger_sys_write(line);
}

/* Membaca VmSize dari modul memory_monitor milik Akmal */
static long get_vmsize_kb(void)
{
    MemoryUsage usage;
    if (read_memory_usage(&usage) != 0)
    {
        return -1;
    }
    return (long)usage.vm_size_kb;
}

/* Membaca VmRSS dari modul memory_monitor milik Akmal */
static long get_vmrss_kb(void)
{
    MemoryUsage usage;
    if (read_memory_usage(&usage) != 0)
    {
        return -1;
    }
    return (long)usage.vm_rss_kb;
}

/* Membuang sisa karakter di baris input agar tidak mengganggu menu utama */
static void discard_line(void)
{
    int c;

    do
    {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

/* Menutup kedua berkas log (System Call dan Standard C Library) */
static void close_logs(void)
{
    logger_sys_close();
    if (std_logger.file != NULL)
    {
        std_logger_close(&std_logger);
    }
}

void run_mmap_munmap(void)
{
    long long count = 0;
    long long i;
    long long shown;
    size_t byte_size;
    int *data;
    long size_before, rss_before;
    long size_after, rss_after;
    long size_unmapped, rss_unmapped;

    /* Membuka kedua log dengan nama metode "mmap" */
    if (std_logger_open(&std_logger, GROUP_PREFIX, "mmap") != 0)
    {
        fprintf(stderr, "Gagal membuka log Standard C Library.\n");
        return;
    }
    if (logger_sys_open("mmap") != 0)
    {
        fprintf(stderr, "Gagal membuka log System Call.\n");
        close_logs();
        return;
    }

    emit("You picked mmap/munmap.\n");

    /* Membaca metrik memori sebelum alokasi */
    size_before = get_vmsize_kb();
    rss_before = get_vmrss_kb();
    emit("VmSize Before: %ld KB | VmRSS Before: %ld KB\n", size_before, rss_before);

    /* Prompt input hanya ditampilkan di layar (tidak masuk ke file log) */
    printf("How many integers to allocate: ");
    fflush(stdout);
    if (scanf("%lld", &count) != 1 || count <= 0 || count > INT_MAX)
    {
        emit("Input tidak valid, jumlah integer harus antara 1 dan %d.\n", INT_MAX);
        discard_line();
        close_logs();
        return;
    }
    discard_line();

    if ((unsigned long long)count > SIZE_MAX / sizeof(int))
    {
        emit("Ukuran alokasi terlalu besar.\n");
        close_logs();
        return;
    }

    byte_size = (size_t)count * sizeof(int);

    /*
     * Alokasi memori menggunakan system call mmap() anonymous:
     * - addr   = NULL : kernel bebas menentukan alamat awal pemetaan.
     * - length = byte_size : ukuran memori dalam byte.
     * - prot   = PROT_READ | PROT_WRITE : izin akses baca dan tulis.
     * - flags  = MAP_PRIVATE | MAP_ANONYMOUS :
     *            MAP_PRIVATE membuat perubahan bersifat privat bagi proses ini,
     *            MAP_ANONYMOUS membuat pemetaan memori murni di RAM tanpa file.
     * - fd     = -1 dan offset = 0 : dipakai karena tidak memetakan file.
     */
    data = (int *)mmap(NULL, byte_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (data == MAP_FAILED)
    {
        emit("mmap gagal mengalokasikan memori: %s\n", strerror(errno));
        close_logs();
        return;
    }

    /*
     * Mengisi array dari N turun ke 1.
     * Penulisan ke halaman memori memicu page fault (demand paging) sehingga
     * kernel mengalokasikan halaman fisik di RAM dan nilai VmRSS meningkat.
     */
    for (i = 0; i < count; i++)
    {
        data[i] = (int)(count - i);
    }

    emit("\nStarting memory address: %p\n", (void *)data);
    emit("Printing first 10 values in reverse order:\n");

    shown = count;
    if (shown > SHOW_COUNT)
    {
        shown = SHOW_COUNT;
    }
    for (i = 0; i < shown; i++)
    {
        emit("%d\n", data[i]);
    }

    /* Membaca metrik memori setelah alokasi dan pengisian array */
    size_after = get_vmsize_kb();
    rss_after = get_vmrss_kb();
    emit("\nVmSize After: %ld KB | VmRSS After: %ld KB\n", size_after, rss_after);

    /*
     * Melepaskan pemetaan memori kembali ke kernel menggunakan munmap().
     * Parameter yang diberikan adalah alamat awal (data) dan ukuran (byte_size).
     */
    if (munmap(data, byte_size) != 0)
    {
        emit("munmap gagal melepas memori: %s\n", strerror(errno));
    }
    else
    {
        size_unmapped = get_vmsize_kb();
        rss_unmapped = get_vmrss_kb();
        emit("VmSize After munmap: %ld KB | VmRSS After munmap: %ld KB\n",
             size_unmapped, rss_unmapped);
    }
    data = NULL;

    close_logs();
}

