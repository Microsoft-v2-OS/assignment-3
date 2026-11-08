#ifndef LOGGER_SYS_H
#define LOGGER_SYS_H

/*
 * Header untuk modul pencatatan log menggunakan System Call Linux. (Adam)
 *
 * Berbeda dengan Standard C Library (fopen, fprintf, fclose) yang memakai
 * pointer struktur FILE* dan memiliki buffer di user-space, modul ini
 * berinteraksi langsung dengan kernel menggunakan file descriptor (integer)
 * melalui system call:
 * - open()  : membuka/membuat file log dan mendapatkan file descriptor (fd)
 * - write() : menulis array byte langsung ke kernel lewat fd
 * - close() : menutup file descriptor setelah selesai digunakan
 *
 * Format nama file log: XX-[metode]_sys-[timestamp].txt
 * dengan [metode] berupa "malloc" atau "mmap".
 */

/* Membuka file log baru untuk metode tertentu ("malloc" atau "mmap").
 * Mengembalikan 0 jika berhasil, atau -1 jika gagal. */
int logger_sys_open(const char *method);

/* Menulis teks ke file log yang sedang terbuka menggunakan system call write(). */
void logger_sys_write(const char *text);

/* Menutup file descriptor log menggunakan system call close(). */
void logger_sys_close(void);

#endif

