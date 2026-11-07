#ifndef MALLOC_HANDLER_H
#define MALLOC_HANDLER_H

/*
 * Header untuk menu 1 (malloc/free). (Gagah)
 *
 * run_malloc_free() dipanggil dari menu di main kalau user pilih opsi 1.
 * Fungsinya jalan sampai selesai sendiri, mulai dari catat metrik awal,
 * minta jumlah integer, malloc dan isi array, tampilkan 10 nilai
 * pertama, sampai free dan catat metrik terakhir. Semua output keluar
 * di layar dan ditulis ke dua log, yang versi fopen dan yang versi open.
 * Detail cara kerjanya ada di malloc_handler.c.
 */
void run_malloc_free(void);

#endif