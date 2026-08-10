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

__attribute__((aligned(32))) uint8_t latx_vmovntdqa_memory[64] = {
    0x42, 0x00, 0xf8, 0x7f, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xf0, 0x7f, 0xef, 0xcd, 0xab, 0x89,
    0x01, 0x00, 0x00, 0x80, 0x10, 0x32, 0x54, 0x76,
    0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00,
};
const uint64_t latx_vmovntdqa_seed[4] = {
    UINT64_C(0xaaaaaaaaaaaaaaaa), UINT64_C(0xbbbbbbbbbbbbbbbb),
    UINT64_C(0xcccccccccccccccc), UINT64_C(0xdddddddddddddddd),
};

extern void latx_avx_single_vmovntdqa_run(uint8_t *output);
extern void latx_avx_single_vmovntdqa_fault_xmm(uint8_t *address);
extern void latx_avx_single_vmovntdqa_fault_ymm(uint8_t *address);

static uint8_t output[LATX_OUTPUT_SIZE];

static long latx_vmovntdqa_syscall6(long number, long arg0, long arg1,
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

static long latx_vmovntdqa_syscall3(long number, long arg0, long arg1,
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

static int latx_vmovntdqa_equal(const uint8_t *left, const uint8_t *right,
                                size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (left[i] != right[i]) {
            return 0;
        }
    }
    return 1;
}

static int latx_vmovntdqa_fault(const char *kind)
{
    uint8_t *page = (uint8_t *)(uintptr_t)latx_vmovntdqa_syscall6(
        LATX_SYS_MMAP, 0, 2 * LATX_PAGE_SIZE,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);
    if ((uintptr_t)page >= (uintptr_t)-4095 ||
        latx_vmovntdqa_syscall3(LATX_SYS_MPROTECT, (long)(page + LATX_PAGE_SIZE),
                                 LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
        return 2;
    }
    uint8_t *address;
    if (kind[4] == 'u') {
        address = page + 1;
    } else if (kind[0] == 'x') {
        address = page + LATX_PAGE_SIZE - 8;
    } else {
        address = page + LATX_PAGE_SIZE - 16;
    }
    if (kind[0] == 'x') {
        latx_avx_single_vmovntdqa_fault_xmm(address);
    } else {
        latx_avx_single_vmovntdqa_fault_ymm(address);
    }
    return 1;
}

int latx_avx_single_main(long argc, char **argv)
{
    if (argc > 1) {
        return latx_vmovntdqa_fault(argv[1]);
    }
    latx_avx_single_vmovntdqa_run(output);
    if (!latx_vmovntdqa_equal(output, output + 64, 32) ||
        !latx_vmovntdqa_equal(output + 32, output + 96, 32)) {
        return 3;
    }
    return latx_avx_single_write_all(output, sizeof(output)) == 0 ? 0 : 1;
}
