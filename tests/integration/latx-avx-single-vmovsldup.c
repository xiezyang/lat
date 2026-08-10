/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9, SYS_MPROTECT = 10, PROT_NONE = 0,
    PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20, PAGE = 4096, OUTPUT_SIZE = 6 * 32,
};

const uint64_t latx_vmovsldup_source_a[4] = {
    UINT64_C(0x7ff8000000000042), UINT64_C(0x7ff0000000000001),
    UINT64_C(0x0123456789abcdef), UINT64_C(0x8000000000000001),
};
const uint64_t latx_vmovsldup_source_b[4] = {
    UINT64_C(0x7ff0000000000042), UINT64_C(0x7ff8000000000001),
    UINT64_C(0xfedcba9876543210), UINT64_C(0x7ffffffffffffffe),
};
const uint64_t latx_vmovsldup_memory[4] = {
    UINT64_C(0x7ff8000000000042), UINT64_C(0x7ff0000000000001),
    UINT64_C(0x0123456789abcdef), UINT64_C(0x8000000000000001),
};

extern void latx_avx_single_vmovsldup_run(uint8_t *);
extern void latx_avx_single_vmovsldup_fault_xmm(uint8_t *);
extern void latx_avx_single_vmovsldup_fault_ymm(uint8_t *);
static uint8_t output[OUTPUT_SIZE];

static long syscall3(long n, long a, long b, long c)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi),
                     "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

static long syscall6(long n, long a, long b, long c, long d, long e, long f)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8") = e;
    register long r9 __asm__("r9") = f;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi),
                     "d"(rdx), "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return rax;
}

static int fault(const char *kind)
{
    uint8_t *page = (uint8_t *)(uintptr_t)syscall6(
        SYS_MMAP, 0, 2 * PAGE, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((uintptr_t)page >= (uintptr_t)-4095 ||
        syscall3(SYS_MPROTECT, (long)(page + PAGE), PAGE, PROT_NONE) < 0) {
        return 2;
    }
    if (kind[0] == 'x') {
        latx_avx_single_vmovsldup_fault_xmm(page + PAGE - 8);
    } else {
        latx_avx_single_vmovsldup_fault_ymm(page + PAGE - 16);
    }
    return 1;
}

int latx_avx_single_main(long argc, char **argv)
{
    if (argc > 1) return fault(argv[1]);
    latx_avx_single_vmovsldup_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) == 0 ? 0 : 1;
}
