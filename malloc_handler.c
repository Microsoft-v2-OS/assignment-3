/*
 * Menu 1, alokasi memori pakai malloc/free.
 *
 * Fungsi run_malloc_free() dipanggil dari menu di main. Pertama buka dua
 * file log, lalu catat VmSize dan VmRSS sebelum alokasi. Setelah itu bakalan
 * user diminta input jumlah integer, array dialokasikan pakai malloc dan
 * diisi dari N turun ke 1. Sepuluh nilai pertama ditampilkan bersama
 * alamat awalnya, lalu metrik dicatat lagi. Terakhir array di free dan
 * metrik dicatat sekali lagi, buat lihat apakah memorinya beneran balik.
 *
 * Soal angka VmSize dan VmRSS, ada beberapa hal yang perlu diingat.
 * malloc itu cuma fungsi library. Yang sebenarnya minta memori ke kernel
 * adalah brk atau mmap, tergantung ukurannya. Kalau ukurannya kecil,
 * glibc ambil dari heap lewat brk. Kalau lebih dari sekitar 128 KB
 * (M_MMAP_THRESHOLD), glibc pakai mmap anonymous. Jadi untuk jumlah
 * integer yang besar, hasilnya akan mirip dengan opsi mmap/munmap.
 *
 * Untuk blok besar, VmSize naik begitu malloc dipanggil tapi VmRSS belum.
 * Kernel baru kasih halaman fisik waktu halamannya pertama kali ditulis
 * (demand paging). Makanya array harus diisi dulu, baru VmRSS ikut naik.
 *
 * Waktu free, blok yang dari mmap langsung dikembalikan ke kernel, jadi
 * VmSize dan VmRSS turun. Blok kecil di heap biasanya tidak dikembalikan.
 * glibc menyimpannya buat dipakai malloc berikutnya, jadi angkanya hampir
 * tidak berubah.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>

#include "malloc_handler.h"
#include "memory_monitor.h"
#include "logger_std.h"
#include "logger_sys.h"

#define SHOW_COUNT 10
#define LINE_BUF_SIZE 256
#define GROUP_ID "04"

/* logger std disimpan di sini supaya emit() bisa memakainya */
static StdLogger std_log;

/*
 * tulis ke layar dan ke dua log dengan teks yang sama. std_logger_printf
 * sudah mencetak ke layar sekaligus ke log std, jadi tidak perlu fputs
 * ke stdout lagi (kalau ditambah, di layar jadi muncul dua kali).
 */
static void emit(const char *fmt, ...)
{
    char line[LINE_BUF_SIZE];
    va_list args;

    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    std_logger_printf(&std_log, "%s", line);
    logger_sys_write(line);
}

/* buang sisa input di buffer biar menu tidak ikut kebaca */
static void discard_line(void)
{
    int c;

    do
    {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

static void close_logs(void)
{
    logger_sys_close();
    std_logger_close(&std_log);
}

/* baca VmSize dan VmRSS lewat modul monitor, kalau gagal nilainya 0 */
static void read_usage(MemoryUsage *usage)
{
    if (read_memory_usage(usage) != 0)
    {
        fprintf(stderr, "Gagal membaca /proc/self/status.\n");
        usage->vm_size_kb = 0;
        usage->vm_rss_kb = 0;
    }
}

void run_malloc_free(void)
{
    long long count = 0;
    long long i;
    long long shown;
    int *data;
    MemoryUsage before, after, freed;

    /* nama metode "malloc" dipakai buat nama file log */
    if (std_logger_open(&std_log, GROUP_ID, "malloc") != 0)
    {
        fprintf(stderr, "Gagal membuka log Standard C Library.\n");
        return;
    }
    if (logger_sys_open("malloc") != 0)
    {
        fprintf(stderr, "Gagal membuka log System Call.\n");
        std_logger_close(&std_log);
        return;
    }

    emit("You picked malloc/free.\n");

    /* metrik sebelum alokasi, dibaca lewat modul monitor */
    read_usage(&before);
    emit("VmSize Before: %llu KB | VmRSS Before: %llu KB\n",
         before.vm_size_kb, before.vm_rss_kb);

    /* prompt cuma muncul di layar, tidak masuk log (sesuai contoh di soal) */
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

    data = (int *)malloc((size_t)count * sizeof(int));
    if (data == NULL)
    {
        emit("malloc gagal mengalokasikan memori.\n");
        close_logs();
        return;
    }

    /*
     * array diisi dari N turun ke 1. Baru di sini halaman fisiknya
     * benar benar dipakai (page fault), makanya VmRSS naik. Kalau cuma
     * malloc tanpa diisi, VmRSS hampir tidak berubah.
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

    /* metrik setelah alokasi dan array terisi */
    read_usage(&after);
    emit("\nVmSize After: %llu KB | VmRSS After: %llu KB\n",
         after.vm_size_kb, after.vm_rss_kb);

    /*
     * blok besar (lewat mmap) langsung dikembalikan ke kernel waktu
     * di free, sedangkan blok kecil di heap biasanya ditahan glibc.
     * Makanya metrik dicek lagi di sini.
     */
    free(data);
    data = NULL;

    read_usage(&freed);
    emit("VmSize After free: %llu KB | VmRSS After free: %llu KB\n",
         freed.vm_size_kb, freed.vm_rss_kb);

    close_logs();
}