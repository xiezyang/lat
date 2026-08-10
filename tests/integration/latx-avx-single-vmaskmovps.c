/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9, SYS_MPROTECT = 10, SYS_RT_SIGACTION = 13, SYS_EXIT = 60,
    PROT_NONE = 0, PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20, SA_SIGINFO = 4, SA_RESTORER = 0x04000000,
    SIGBUS = 7, SIGSEGV = 11, PAGE = 4096, PAGES = 3, CASES = 12,
    RECORD_SIZE = 64, OUTPUT_SIZE = CASES * RECORD_SIZE, STORE_BASE = 1024,
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

extern void latx_avx_single_vmaskmovps_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vmaskmovps_fault_load(uint8_t *, uint8_t *);
extern void latx_avx_single_vmaskmovps_fault_store(uint8_t *, uint8_t *);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t output[OUTPUT_SIZE];

static inline long syscall3(long n, long a, long b, long c)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi),
                     "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

static inline long syscall4(long n, long a, long b, long c, long d)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    register long r10 __asm__("r10") = d;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi),
                     "d"(rdx), "r"(r10) : "rcx", "r11", "memory");
    return rax;
}

static inline long syscall6(long n, long a, long b, long c, long d, long e,
                             long f)
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

static __attribute__((noreturn)) void exit_now(int status)
{
    __asm__ volatile("syscall" : : "a"((long)SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void handler(int sig, void *info, void *ctx)
{
    (void)info;
    (void)ctx;
    exit_now(128 + sig);
}

static int install_fault_handlers(void)
{
    struct kernel_sigaction action = {
        handler, SA_SIGINFO | SA_RESTORER, latx_avx_single_rt_sigreturn, 0,
    };
    return syscall4(SYS_RT_SIGACTION, SIGSEGV, (long)(uintptr_t)&action, 0,
                    8) < 0 ||
           syscall4(SYS_RT_SIGACTION, SIGBUS, (long)(uintptr_t)&action, 0,
                    8) < 0;
}

static void put32(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        p[i] = (uint8_t)(value >> (8 * i));
}

static void prepare(uint8_t *p)
{
    static const uint32_t source[8] = {
        UINT32_C(0x11223344), UINT32_C(0x55667788), UINT32_C(0x99aabbcc),
        UINT32_C(0xddeeff00), UINT32_C(0x01234567), UINT32_C(0x89abcdef),
        UINT32_C(0xfedcba98), UINT32_C(0x76543210),
    };
    static const uint32_t data[8] = {
        UINT32_C(0x0f1e2d3c), UINT32_C(0x4b5a6978), UINT32_C(0x88776655),
        UINT32_C(0x44332211), UINT32_C(0x10203040), UINT32_C(0x50607080),
        UINT32_C(0x90807060), UINT32_C(0x50403020),
    };
    static const uint32_t mask_zero[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static const uint32_t mask_one[8] = {
        UINT32_C(0x80000000), UINT32_C(0x80000000), UINT32_C(0x80000000),
        UINT32_C(0x80000000), UINT32_C(0x80000000), UINT32_C(0x80000000),
        UINT32_C(0x80000000), UINT32_C(0x80000000),
    };
    static const uint32_t mask_mix[8] = {
        UINT32_C(0x80000000), 0, UINT32_C(0x80000000), 0,
        UINT32_C(0x80000000), 0, UINT32_C(0x80000000), 0,
    };

    latx_avx_single_fill(p, PAGES * PAGE, UINT64_C(0x564d41534b5053));
    for (unsigned i = 0; i < 8; ++i) {
        put32(p + i * 4, source[i]);
        put32(p + 384 + i * 4, data[i]);
        put32(p + 256 + i * 4, mask_zero[i]);
        put32(p + 288 + i * 4, mask_one[i]);
        put32(p + 320 + i * 4, mask_mix[i]);
        put32(p + 448 + i * 4, UINT32_C(0xa5a5a5a5));
    }
    for (unsigned i = 0; i < 6; ++i)
        latx_avx_single_fill(p + STORE_BASE + i * 64, 64,
                             UINT64_C(0x53544f52505053) + i);
}

int latx_avx_single_main(long argc, char **argv)
{
    long map = syscall6(SYS_MMAP, 0, PAGES * PAGE, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint8_t *p;

    if (map < 0)
        return 70;
    p = (uint8_t *)(uintptr_t)map;
    prepare(p);
    if (argc == 1 || (argc == 2 && argv[1][0] == 'r')) {
        latx_avx_single_vmaskmovps_run(output, p);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc != 2 || (argv[1][0] < 'a' || argv[1][0] > 'd'))
        return 72;
    if (install_fault_handlers())
        return 73;
    if (syscall3(SYS_MPROTECT, (long)(uintptr_t)(p + PAGE), PAGE,
                 PROT_NONE) < 0)
        return 71;
    p += PAGE - 8;
    if (argv[1][0] == 'a' || argv[1][0] == 'c') {
        latx_avx_single_vmaskmovps_fault_load(
            p, (uint8_t *)(uintptr_t)map + (argv[1][0] == 'a' ? 256 : 288));
        return argv[1][0] == 'a' ? 0 : 90;
    }
    latx_avx_single_vmaskmovps_fault_store(
        p, (uint8_t *)(uintptr_t)map + (argv[1][0] == 'b' ? 256 : 288));
    return argv[1][0] == 'b' ? 0 : 90;
}
