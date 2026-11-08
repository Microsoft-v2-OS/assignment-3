"""Uji kontrak modul dengan GCC native; bukan pengganti pengukuran Linux.

Jalankan: python tests/check_demand_paging.py
Header fixture dibuat sementara, tidak menggantikan modul anggota kelompok.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADERS = {
    "memory_monitor.h": "long mem_get_vmsize_kb(void); long mem_get_vmrss_kb(void);",
    "logger_std.h": "int logger_std_open(const char *); void logger_std_write(const char *); void logger_std_close(void);",
    "logger_sys.h": "int logger_sys_open(const char *); void logger_sys_write(const char *); void logger_sys_close(void);",
    "unistd.h": "#define _SC_PAGESIZE 30\nlong sysconf(int);",
    "sys/mman.h": """
#include <stddef.h>
#define MAP_FAILED ((void *)-1)
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 32
#define PROT_READ 1
#define PROT_WRITE 2
#define MADV_NOHUGEPAGE 15
void *mmap(void *, size_t, int, int, int, long);
int munmap(void *, size_t);
int madvise(void *, size_t, int);
""",
}
HARNESS = r'''
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "demand_paging.h"
#include "sys/mman.h"
static const size_t mib = 1024 * 1024;
static unsigned char *memory;
static int mode, std_closed, sys_closed, unmapped, reads;
static char std_log[16384], sys_log[16384];
int logger_std_open(const char *method) {
    assert(strcmp(method, "mmap") == 0); return mode == 1 ? -1 : 0;
}
int logger_sys_open(const char *method) {
    assert(strcmp(method, "mmap") == 0); return mode == 2 ? -1 : 0;
}
void logger_std_write(const char *s) { assert(strlen(std_log)+strlen(s)<sizeof std_log); strcat(std_log,s); }
void logger_sys_write(const char *s) { assert(strlen(sys_log)+strlen(s)<sizeof sys_log); strcat(sys_log,s); }
void logger_std_close(void) { std_closed++; }
void logger_sys_close(void) { sys_closed++; }
long sysconf(int name) { (void)name; return mode == 3 ? 8192 : 4096; }
long mem_get_vmsize_kb(void) {
    reads++;
    if (mode == 4 || (mode == 6 && reads == 4)) return -1;
    return memory ? 1048576 + 2000 : 2000;
}
long mem_get_vmrss_kb(void) {
    size_t offset, pages = 0;
    if (memory) for (offset = 0; offset < 100*mib; offset += 4096)
        if (memory[offset]) pages++;
    return 1000 + (long)pages*4;
}
void *mmap(void *addr, size_t len, int prot, int flags, int fd, long off) {
    assert(addr == NULL && len == 1024*mib);
    assert(prot == (PROT_READ|PROT_WRITE));
    assert(flags == (MAP_PRIVATE|MAP_ANONYMOUS) && fd == -1 && off == 0);
    if (mode == 5) { errno = ENOMEM; return MAP_FAILED; }
    /* Hanya area yang boleh disentuh disediakan agar overrun terdeteksi. */
    memory = calloc(100*mib, 1); assert(memory); return memory;
}
int madvise(void *addr, size_t len, int advice) {
    assert(addr == memory && len == 1024*mib && advice == MADV_NOHUGEPAGE);
    if (mode == 7) { errno = EINVAL; return -1; } return 0;
}
int munmap(void *addr, size_t len) {
    size_t i;
    assert(addr == memory && len == 1024*mib);
    if (mode == 0 || mode == 7 || mode == 8) {
        for (i = 0; i < 100*mib; i++)
            assert(memory[i] == (i % 4096 == 0 ? 1 : 0));
    }
    unmapped++;
    if (mode == 8) { errno = EINVAL; return -1; }
    free(memory); memory = NULL; return 0;
}
int main(int argc, char **argv) {
    const size_t stages[] = {0,5,10,25,50,75,100};
    size_t i, touched, pages;
    long size, rss, delta;
    const char *line;
    assert(argc == 2); mode = atoi(argv[1]); run_demand_paging();
    assert(std_closed == (mode == 1 ? 0 : 1));
    assert(sys_closed == (mode == 1 || mode == 2 ? 0 : 1));
    assert(strcmp(std_log,sys_log) == 0);
    assert(unmapped == (mode >= 6 || mode == 0 ? 1 : 0));
    if (mode == 0 || mode == 7 || mode == 8) {
        line = strstr(std_log,"Disentuh (MiB)"); assert(line);
        line = strchr(line,'\n')+1;
        line = strchr(line,'\n')+1; /* ringkasan sesudah mmap */
        for (i = 0; i < 7; i++) {
            assert(sscanf(line,"%zu %zu %ld %ld %ld",&touched,&pages,&size,&rss,&delta)==5);
            assert(touched == stages[i] && pages == stages[i]*256);
            assert(size == 1050576 && rss == 1000+(long)stages[i]*1024);
            assert(delta == (i ? (long)(stages[i]-stages[i-1])*1024 : 0));
            line = strchr(line,'\n')+1;
        }
        assert(strstr(std_log,"Total delta VmRSS sejak 0 MiB: +102400 KB"));
    }
    if (mode == 4 || mode == 6) assert(strstr(std_log,"Gagal membaca"));
    if (mode == 5) assert(strstr(std_log,"mmap gagal"));
    if (mode == 7) assert(strstr(std_log,"MADV_NOHUGEPAGE gagal"));
    if (mode == 8) { assert(strstr(std_log,"munmap gagal")); free(memory); }
    return 0;
}
'''

gcc = shutil.which("gcc")
if not gcc:
    raise SystemExit("GCC tidak ditemukan di PATH.")
with tempfile.TemporaryDirectory(prefix="demand-paging-") as directory:
    temp = Path(directory)
    for name, source in HEADERS.items():
        path = temp / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(source, encoding="utf-8")
    harness = temp / "harness.c"
    harness.write_text(HARNESS, encoding="utf-8")
    executable = temp / "check.exe"
    subprocess.run([gcc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I", str(temp), "-I", str(ROOT),
                    str(ROOT / "demand_paging.c"), str(harness),
                    "-o", str(executable)], check=True)
    for mode in range(9):
        result = subprocess.run([str(executable), str(mode)], capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(f"Kasus {mode} gagal:\n{result.stdout}\n{result.stderr}")
    print("LULUS: 9 kasus kontrak, isi halaman, tabel, dua log, dan cleanup.")
