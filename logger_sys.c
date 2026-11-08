/*
 * Implementasi pencatatan log menggunakan System Call (open, write, close). (Adam)
 *
 * Cara kerja modul ini:
 * 1. logger_sys_open() membentuk nama file sesuai format soal:
 *    XX-[metode]_sys-[timestamp].txt
 *    lalu memanggil system call open() untuk mendapatkan file descriptor (log_fd).
 * 2. logger_sys_write() mengirimkan string ke file log menggunakan system call
 *    write() berdasarkan panjang string (strlen).
 * 3. logger_sys_close() menutup file descriptor menggunakan system call close()
 *    dan mengatur ulang log_fd menjadi -1.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "logger_sys.h"

/* Nomor kelompok untuk awalan nama file log*/
#ifndef GROUP_PREFIX
#define GROUP_PREFIX "04"
#endif

/*
 * File descriptor disimpan sebagai variabel static di dalam modul ini.
 * Nilai -1 menandakan belum ada file log yang sedang dibuka.
 */
static int log_fd = -1;

int logger_sys_open(const char *method)
{
    char timestamp[32];
    char filename[128];
    time_t now;
    struct tm *tm_info;

    if (method == NULL || method[0] == '\0')
    {
        return -1;
    }

    /* Jika masih ada file log yang terbuka sebelumnya, tutup terlebih dahulu */
    if (log_fd >= 0)
    {
        close(log_fd);
        log_fd = -1;
    }

    /* Mengambil waktu saat ini untuk dijadikan timestamp pada nama file */
    now = time(NULL);
    tm_info = localtime(&now);
    if (tm_info != NULL)
    {
        strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", tm_info);
    }
    else
    {
        snprintf(timestamp, sizeof(timestamp), "%ld", (long)now);
    }

    /* Membentuk nama file: XX-[metode]_sys-[timestamp].txt */
    snprintf(filename, sizeof(filename), "%s-%s_sys-%s.txt",
             GROUP_PREFIX, method, timestamp);

    /*
     * Membuka file menggunakan system call open():
     * - O_WRONLY : buka file hanya untuk penulisan (write-only)
     * - O_CREAT  : buat file baru jika belum ada
     * - O_APPEND : tambahkan teks di akhir file (agar aman jika dibuka di detik yang sama)
     * - 0644     : izin akses file (rw-r--r--: pemilik bisa baca/tulis, lainnya baca)
     */
    log_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0)
    {
        return -1;
    }

    return 0;
}

void logger_sys_write(const char *text)
{
    size_t len;
    ssize_t written;

    /* Pastikan file sudah terbuka dan teks yang dikirim valid */
    if (log_fd < 0 || text == NULL)
    {
        return;
    }

    len = strlen(text);
    if (len == 0)
    {
        return;
    }

    /*
     * System call write(fd, buffer, count):
     * Menulis sebanyak 'len' byte dari 'text' langsung ke kernel melalui 'log_fd'.
     */
    written = write(log_fd, text, len);
    (void)written;
}

void logger_sys_close(void)
{
    /*
     * System call close(fd):
     * Menutup file descriptor di kernel dan mereset log_fd kembali ke -1.
     */
    if (log_fd >= 0)
    {
        close(log_fd);
        log_fd = -1;
    }
}

