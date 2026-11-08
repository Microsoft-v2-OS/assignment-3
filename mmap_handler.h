#ifndef MMAP_HANDLER_H
#define MMAP_HANDLER_H

/*
 * Header untuk menu 2 (mmap/munmap). (Adam)
 *
 * run_mmap_munmap() dipanggil dari menu di main kalau user pilih opsi 2.
 * Fungsinya berjalan dari awal sampai selesai: membuka kedua file log,
 * mencatat metrik awal (VmSize dan VmRSS), meminta input jumlah integer,
 * mengalokasikan memori menggunakan mmap() anonymous, mengisi array dari
 * N turun ke 1, menampilkan alamat awal serta 10 nilai pertama, mencatat
 * metrik setelah alokasi, lalu melepas memori dengan munmap() dan mencatat
 * metrik terakhir.
 */
void run_mmap_munmap(void);

#endif

