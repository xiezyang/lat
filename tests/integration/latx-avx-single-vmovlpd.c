/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    WI1904_SYS_MMAP = 9,
    WI1904_SYS_MPROTECT = 10,
    WI1904_PROT_READ = 1,
    WI1904_PROT_WRITE = 2,
    WI1904_MAP_PRIVATE = 2,
    WI1904_MAP_ANONYMOUS = 0x20,
    WI1904_PAGE_SIZE = 4096,
    WI1904_RECORD_SIZE = 48,
    WI1904_RECORDS = 4,
    WI1904_OUTPUT_SIZE = WI1904_RECORD_SIZE * WI1904_RECORDS,
};

const uint64_t wi1904_vmovlpd_source_a[2] = {
    UINT64_C(0x7ff8000000000042), UINT64_C(0x8000000000000001),
};
const uint64_t wi1904_vmovlpd_source_b[2] = {
    UINT64_C(0x7ff0000000000001), UINT64_C(0x0123456789abcdef),
};
const uint64_t wi1904_vmovlpd_seed[4] = {
    UINT64_C(0xaaaaaaaaaaaaaaaa), UINT64_C(0xbbbbbbbbbbbbbbbb),
    UINT64_C(0x7ff0000000000042), UINT64_C(0xcccccccccccccccc),
};

static uint8_t output[WI1904_OUTPUT_SIZE];
static uint8_t memory[32] = {
    0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

extern void latx_avx_single_vmovlpd_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vmovlpd_fault_load(uint8_t *);
extern void latx_avx_single_vmovlpd_fault_store(uint8_t *);

static inline long wi1904_syscall6(long number, long arg0, long arg1,
                                   long arg2, long arg3, long arg4,
                                   long arg5)
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

static int wi1904_streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

int latx_avx_single_main(long argc, char **argv)
{
    if (argc == 1) {
        latx_avx_single_vmovlpd_run(output, memory);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc != 2 ||
        (!wi1904_streq(argv[1], "fault-load") &&
         !wi1904_streq(argv[1], "fault-store"))) {
        return 2;
    }

    long mapping = wi1904_syscall6(
        WI1904_SYS_MMAP, 0, 2 * WI1904_PAGE_SIZE,
        WI1904_PROT_READ | WI1904_PROT_WRITE,
        WI1904_MAP_PRIVATE | WI1904_MAP_ANONYMOUS, -1, 0);
    if (mapping < 0) {
        return 3;
    }
    uint8_t *page = (uint8_t *)(uintptr_t)mapping;
    if (wi1904_syscall6(WI1904_SYS_MPROTECT,
                        (long)(uintptr_t)(page + WI1904_PAGE_SIZE),
                        WI1904_PAGE_SIZE, 0, 0, 0, 0) < 0) {
        return 4;
    }
    uint8_t *crossing = page + WI1904_PAGE_SIZE - 4;
    if (wi1904_streq(argv[1], "fault-load")) {
        latx_avx_single_vmovlpd_fault_load(crossing);
    } else {
        latx_avx_single_vmovlpd_fault_store(crossing);
    }
    return 5;
}
