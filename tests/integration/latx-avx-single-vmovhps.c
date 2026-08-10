/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    LATX_SYS_MMAP = 9,
    LATX_SYS_MPROTECT = 10,
    LATX_PROT_NONE = 0,
    LATX_PROT_READ = 1,
    LATX_PROT_WRITE = 2,
    LATX_MAP_PRIVATE = 2,
    LATX_MAP_ANONYMOUS = 0x20,
    LATX_PAGE_SIZE = 4096,
    LATX_OUTPUT_SIZE = 4 * 32,
};

const uint64_t latx_vmovhps_source_a[4] = {
    UINT64_C(0x7ff8000000000042), UINT64_C(0x7ff0000000000001),
    UINT64_C(0x0123456789abcdef), UINT64_C(0x8000000000000001),
};
const uint64_t latx_vmovhps_source_b[4] = {
    UINT64_C(0x7ff0000000000042), UINT64_C(0x7ff8000000000001),
    UINT64_C(0xfedcba9876543210), UINT64_C(0x7ffffffffffffffe),
};
const uint64_t latx_vmovhps_memory[4] = {
    UINT64_C(0x7ff8000000000042), UINT64_C(0x7ff0000000000001),
    UINT64_C(0xaaaaaaaaaaaaaaaa), UINT64_C(0xbbbbbbbbbbbbbbbb),
};
const uint64_t latx_vmovhps_seed[4] = {
    UINT64_C(0xaaaaaaaaaaaaaaaa), UINT64_C(0xbbbbbbbbbbbbbbbb),
    UINT64_C(0xcccccccccccccccc), UINT64_C(0xdddddddddddddddd),
};
const uint64_t latx_vmovhps_store_seed[4] = {
    UINT64_C(0x1111111111111111), UINT64_C(0x2222222222222222),
    UINT64_C(0x3333333333333333), UINT64_C(0x4444444444444444),
};
uint64_t latx_vmovhps_store_area[4] = {
    UINT64_C(0x1111111111111111), UINT64_C(0x2222222222222222),
    UINT64_C(0x3333333333333333), UINT64_C(0x4444444444444444),
};

extern void latx_avx_single_vmovhps_run(uint8_t *output);
extern void latx_avx_single_vmovhps_fault_load(uint8_t *address);
extern void latx_avx_single_vmovhps_fault_store(uint8_t *address);

static uint8_t output[LATX_OUTPUT_SIZE];

static long latx_vmovhps_syscall6(long number, long arg0, long arg1,
                                   long arg2, long arg3, long arg4, long arg5)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = arg0;
    register long rsi __asm__("rsi") = arg1;
    register long rdx __asm__("rdx") = arg2;
    register long r10 __asm__("r10") = arg3;
    register long r8 __asm__("r8") = arg4;
    register long r9 __asm__("r9") = arg5;

    __asm__ volatile("syscall"
                     : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10),
                       "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return rax;
}

static long latx_vmovhps_syscall3(long number, long arg0, long arg1,
                                   long arg2)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = arg0;
    register long rsi __asm__("rsi") = arg1;
    register long rdx __asm__("rdx") = arg2;

    __asm__ volatile("syscall"
                     : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx)
                     : "rcx", "r11", "memory");
    return rax;
}

static int latx_vmovhps_equal(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static int latx_vmovhps_fault(const char *kind)
{
    uint8_t *page = (uint8_t *)(uintptr_t)latx_vmovhps_syscall6(
        LATX_SYS_MMAP, 0, 2 * LATX_PAGE_SIZE,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);

    if ((uintptr_t)page >= (uintptr_t)-4095 ||
        latx_vmovhps_syscall3(LATX_SYS_MPROTECT, (long)(page + LATX_PAGE_SIZE),
                              LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
        return 2;
    }
    uint8_t *cross_page = page + LATX_PAGE_SIZE - 4;
    if (latx_vmovhps_equal(kind, "load-cross")) {
        latx_avx_single_vmovhps_fault_load(cross_page);
    } else {
        latx_avx_single_vmovhps_fault_store(cross_page);
    }
    return 1;
}

int latx_avx_single_main(long argc, char **argv)
{
    if (argc > 1) {
        return latx_vmovhps_fault(argv[1]);
    }
    latx_avx_single_vmovhps_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) == 0 ? 0 : 1;
}
