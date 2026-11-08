#ifndef DEMAND_PAGING_H
#define DEMAND_PAGING_H

/* Padjri: dipanggil dari opsi 3 pada main, seperti run_malloc_free().
 * Memetakan 1 GiB, menyentuh sampai 100 MiB, lalu melepas pemetaan.
 * Memakai memory_monitor serta kedua logger yang sama dengan menu malloc.
 * Target eksekusi: Linux dengan halaman dasar 4096 byte.
 */
void run_demand_paging(void);

#endif
