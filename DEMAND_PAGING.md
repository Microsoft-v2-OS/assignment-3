# Bagian Padjri: Varian 4 - Demand Paging

## Acuan dan batas pekerjaan

Implementasi mengikuti Varian 4 pada halaman 6 soal dan pembagian tugas Padjri:
alokasi virtual besar, penulisan bertahap tiap 4 KB, pengambilan VmSize/VmRSS,
perhitungan halaman serta delta RSS, dan tabel hasil percobaan.

Saat folder diperiksa, pekerjaan anggota lain yang tersedia hanya
`malloc_handler.c/.h` (Gagah). Modul monitor Akmal, logger Akmal/Adam,
`mmap_handler`, `banner`, `main`, dan Makefile belum ada. Modul ini menggunakan
kontrak fungsi yang sudah dipakai kode Gagah; tidak menggantikan implementasi
anggota lain. Program kelompok lengkap belum bisa dibangun dari folder ini saja.

## Integrasi

Tambahkan pada `main.c`:

```c
#include "demand_paging.h"

/* Dalam switch menu yang sudah ada: */
case 3:
    run_demand_paging();
    break;
```

Tambahkan `demand_paging.c` ke daftar sumber untuk **kedua** target Makefile
(dynamic dan static). Gunakan Linux, misalnya Ubuntu/WSL, dengan GCC dan
halaman dasar 4096 byte. MinGW Windows tidak menyediakan `/proc/self/status`
dan anonymous mmap Linux yang dibutuhkan tugas ini.

Kontrak dependensi, mengikuti pemanggilan di `malloc_handler.c`:

| Header | Fungsi yang dipakai |
| --- | --- |
| `memory_monitor.h` | `mem_get_vmsize_kb()`, `mem_get_vmrss_kb()` mengembalikan `long` dalam KB; nilai negatif saat gagal |
| `logger_std.h` | `logger_std_open(const char *metode)` mengembalikan 0 jika berhasil; `logger_std_write(const char *teks)`, `logger_std_close()` |
| `logger_sys.h` | `logger_sys_open(const char *metode)` mengembalikan 0 jika berhasil; `logger_sys_write(const char *teks)`, `logger_sys_close()` |

Penanganan kegagalan penulisan log tetap menjadi tanggung jawab implementasi
logger, karena kode malloc yang tersedia tidak menggunakan hasil fungsi write.
Modul ini membuka logger dengan metode `mmap`, sehingga tetap mengikuti format
`XX-mmap_std-[timestamp].txt` dan `XX-mmap_sys-[timestamp].txt` pada soal.
Judul `Varian 4` membedakan isinya dari percobaan menu 2. Pastikan logger
membuat nama unik atau menambahkan data jika dua percobaan terjadi pada timestamp
yang sama, supaya log sebelumnya tidak tertimpa.

## Alur percobaan dan bahan demo

1. Buka kedua logger, periksa ukuran halaman, dan ambil metrik sebelum alokasi.
2. Pesan **1 GiB = 1024 MiB = 1.073.741.824 byte** melalui `mmap` dengan
   `MAP_PRIVATE | MAP_ANONYMOUS`, `PROT_READ | PROT_WRITE`, fd -1, offset 0.
   Tidak memakai `MAP_POPULATE` dan tidak mengisi seluruh blok di awal.
3. Jika tersedia, gunakan `MADV_NOHUGEPAGE` pada pemetaan ini untuk mengurangi
   pengaruh Transparent Huge Pages. Kegagalannya dilaporkan, demo tetap berjalan.
4. Ambil metrik ketika 0 halaman disentuh, lalu tulis satu byte pada setiap
   offset kelipatan 4096. Tahap kumulatifnya 5, 10, 25, 50, 75, dan 100 MiB,
   sesuai rentang contoh soal. Setiap tahap melanjutkan tahap sebelumnya.
5. Tampilkan tabel jumlah halaman, VmSize, VmRSS, dan delta RSS per tahap;
   kirim output yang sama ke kedua logger. Setelah itu tampilkan delta RSS total.
6. Lepas **seluruh 1 GiB** dengan `munmap`, baca metrik kembali, tutup kedua log,
   lalu kembali ke menu pemanggil. Kegagalan pembacaan metrik setelah mmap juga
   melewati jalur cleanup ini.

Seratus MiB yang disentuh berarti 25.600 halaman. Walaupun hanya satu byte per
halaman ditulis (25.600 byte store), seluruh halaman tersebut perlu didukung
memori fisik. Blok pemetaan tetap 1 GiB; 924 MiB sisanya belum disentuh.
Pointer `volatile` memastikan penulisan eksperimen tidak dibuang optimizer.

Dalam kondisi normal tanpa tekanan memori, VmSize naik sekitar 1.048.576 KB
setelah mmap dan relatif tetap selama penulisan. VmRSS mula-mula hanya berubah
sedikit, kemudian meningkat ketika write fault membuat kernel menyediakan
halaman fisik. Pada penulisan 5 MiB pertama, pertambahan RSS diperkirakan sekitar
5120 KB; delta pada tahap 10 ke 25 MiB sekitar 15360 KB. Kolom delta membandingkan
dengan **baris sebelumnya**, sedangkan ringkasan total membandingkan dengan
baris **0 MiB**. Tidak ada angka RSS hasil pengukuran yang di-hardcode.

VmRSS dari `/proc/self/status` merupakan metrik proses dan pencatatannya bisa
tertunda atau kurang presisi. Library, aktivitas logger, THP, reclaim, dan swap
juga dapat memengaruhi hasil. Kenaikan tidak harus persis sama dengan contoh;
jumlah halaman pada tabel adalah halaman yang **ditulis oleh program**, bukan
hasil menghitung page fault atau jumlah halaman yang pasti masih resident.
Tidak ada retry yang memaksa nilai RSS sesuai harapan. Setelah munmap, ruang
alamat dilepas dan RSS umumnya turun, tetapi tidak harus persis ke baseline.

Untuk presentasi: tunjukkan baseline, kenaikan VmSize tanpa sentuhan, kenaikan
VmRSS seiring halaman disentuh, dan hasil setelah munmap. Buka kedua berkas log
untuk membuktikan tabel serta alamat pemetaan tercatat. Gunakan hasil Linux
yang sebenarnya sebagai data mentah laporan.

## Verifikasi

```sh
python tests/check_demand_paging.py
```

Pengujian memakai GCC `-std=c11 -O2 -Wall -Wextra -Werror` dengan header dan
dependensi simulasi sementara. Sembilan kasus memeriksa jalur normal, kegagalan
kedua pembukaan log, ukuran halaman yang tidak cocok, kegagalan monitor sebelum
dan setelah alokasi, kegagalan mmap, kegagalan madvise, dan kegagalan munmap.
Pengujian memeriksa byte yang ditulis pada setiap halaman, urutan tahap, jumlah
halaman, delta per tahap, kesamaan dua log, dan pelepasan resource.

**Status:** sembilan kasus lulus di Windows/MinGW. Ini memvalidasi logika dan
kontrak integrasi, bukan bukti demand paging kernel. Pengukuran Linux, linking
dengan modul anggota lain, kompilasi static/dynamic, dan demo menu lengkap masih
perlu dijalankan setelah dependensi kelompok tersedia. WSL yang tersedia saat
pengerjaan hanya `docker-desktop`, tanpa compiler C; CLI Docker di distro
internal tersebut juga tidak mendukung eksekusi container langsung.
