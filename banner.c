#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "banner.h"

#define CHOICE_BUF_SIZE 64

void print_banner(void)
{
    printf("=== KELOMPOK 04 - TUGAS MEMORI ===\n");
    printf("Kelas B | Sistem Operasi CSCM602055 | Gasal 2026/2027\n");
    printf("Anggota: Adam Wahyu Syaputra, Muhammad Akmal Haqqani,\n");
    printf("         Muhammad Rifky Padjri, Nugraha Kautsarrizqi Caksana\n\n");
}

void print_menu(void)
{
    printf("1. Allocate with malloc/free\n");
    printf("2. Allocate with mmap/munmap\n");
    printf("3. Eksekusi Fitur Variasi Kelompok (Varian 4, Demand Paging)\n");
    printf("4. Exit\n\n");
    printf("Input: ");
    fflush(stdout);
}

int read_menu_choice(void)
{
    char buf[CHOICE_BUF_SIZE];
    char *end;
    long value;

    if (fgets(buf, sizeof(buf), stdin) == NULL)
    {
        /* EOF, misalnya input dialihkan dari file yang sudah habis. */
        return MENU_EXIT;
    }

    /* Kalau baris lebih panjang dari buffer, buang sisanya supaya tidak
     * terbaca sebagai pilihan menu berikutnya. */
    if (strchr(buf, '\n') == NULL)
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
        {
        }
        return MENU_INVALID;
    }

    value = strtol(buf, &end, 10);
    if (end == buf)
    {
        return MENU_INVALID;
    }
    while (*end == ' ' || *end == '\t')
    {
        end++;
    }
    if (*end != '\n' || value < 1 || value > 4)
    {
        return MENU_INVALID;
    }
    return (int)value;
}