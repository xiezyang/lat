/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { OUTPUT_SIZE = 12 * 32 };
enum { SYS_MMAP = 9, SYS_MPROTECT = 10, PROT_READ = 1, PROT_WRITE = 2 };
static uint8_t output[OUTPUT_SIZE];
extern void latx_avx_single_vmulss_run(uint8_t *);
extern void latx_avx_single_vmulss_fault_xmm(uint8_t *);


static inline long syscall6(long number, long a0, long a1, long a2,
                            long a3, long a4, long a5)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = a0;
    register long rsi __asm__("rsi") = a1;
    register long rdx __asm__("rdx") = a2;
    register long r10 __asm__("r10") = a3;
    register long r8 __asm__("r8") = a4;
    register long r9 __asm__("r9") = a5;
    __asm__ volatile("syscall" : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10),
                       "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return rax;
}

static int streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) { ++left; ++right; }
    return *left == *right;
}

int latx_avx_single_main(long argc, char **argv)
{
    if (argc == 1 || (argc == 2 && streq(argv[1], "reference"))) {
        latx_avx_single_vmulss_run(output);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc != 2 || (!streq(argv[1], "fault-xmm") &&
                      !streq(argv[1], "fault-ymm"))) return 72;
    long mapping = syscall6(SYS_MMAP, 0, 8192, PROT_READ | PROT_WRITE,
                             0x22, -1, 0);
    if (mapping < 0) return 70;
    uint8_t *page = (uint8_t *)(uintptr_t)mapping;
    if (syscall6(SYS_MPROTECT, (long)(uintptr_t)(page + 4096), 4096,
                 0, 0, 0, 0) < 0) return 71;
    if (streq(argv[1], "fault-xmm")) {
        latx_avx_single_vmulss_fault_xmm(page + 4096 - 4 + 1);
        return 90;
    }
    return 72;
}
