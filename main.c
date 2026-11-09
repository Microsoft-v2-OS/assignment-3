#include <stdio.h>

#include "banner.h"
#include "malloc_handler.h"
#include "mmap_handler.h"
#include "demand_paging.h"

int main(void)
{
    int choice;

    for (;;)
    {
        print_banner();
        print_menu();
        choice = read_menu_choice();

        switch (choice)
        {
        case MENU_MALLOC:
            run_malloc_free();
            break;
        case MENU_MMAP:
            run_mmap_munmap();
            break;
        case MENU_VARIAN4:
            run_demand_paging();
            break;
        case MENU_EXIT:
            printf("\nSampai jumpa.\n");
            return 0;
        default:
            printf("\nPilihan tidak valid, ketik angka 1 sampai 4.\n");
            break;
        }
        printf("\n");
    }
}