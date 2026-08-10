/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"

enum { SYS_MMAP = 9, SYS_MPROTECT = 10, PROT_READ = 1, PROT_WRITE = 2,
       PROT_NONE = 0, MAP_PRIVATE = 2, MAP_ANONYMOUS = 0x20,
       PAGE = 4096, OUTPUT_SIZE = 256 };

extern void latx_avx_single_vpshufhw_run(uint8_t *);
extern void latx_avx_single_vpshufhw_fault(uint8_t *);
static uint8_t output[OUTPUT_SIZE];

static inline long syscall6(long n, long a, long b, long c, long d, long e, long f)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8") = e;
    register long r9 __asm__("r9") = f;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx),
                     "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return rax;
}

int latx_avx_single_main(long argc, char **argv)
{
    if (argc <= 1 || (argc == 2 && argv[1][0] == 'n')) {
        latx_avx_single_vpshufhw_run(output);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc == 2 && argv[1][0] == 'f') {
        long mapping = syscall6(SYS_MMAP, 0, PAGE * 2, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapping < 0) return 71;
        if (latx_avx_single_syscall3(SYS_MPROTECT, mapping + PAGE, PAGE,
                                     PROT_NONE) < 0) return 72;
        latx_avx_single_vpshufhw_fault((uint8_t *)(mapping + PAGE - 16));
        return 90;
    }
    return 2;
}
