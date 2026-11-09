#ifndef BANNER_H
#define BANNER_H

/*
 * Modul antarmuka program Tugas 3 Memory in C (Kelompok B04).
 */

/* Pilihan menu. Nilainya sama dengan angka yang diketik user. */
enum menu_choice
{
    MENU_MALLOC = 1,
    MENU_MMAP = 2,
    MENU_VARIAN4 = 3,
    MENU_EXIT = 4,
    MENU_INVALID = 0
};

/* Cetak identitas kelompok. */
void print_banner(void);

/* Cetak daftar pilihan menu beserta prompt "Input: ". */
void print_menu(void);

/* Baca satu baris dari stdin lalu ubah jadi pilihan menu. Mengembalikan
 * MENU_INVALID kalau input bukan angka 1 sampai 4, dan MENU_EXIT kalau
 * stdin sudah habis (EOF) supaya program tidak berputar selamanya. */
int read_menu_choice(void);

#endif