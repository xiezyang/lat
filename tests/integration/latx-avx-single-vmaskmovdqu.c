/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9, SYS_MPROTECT = 10, SYS_RT_SIGACTION = 13, SYS_EXIT = 60,
    PROT_NONE = 0, PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20, SA_SIGINFO = 4, SA_RESTORER = 0x04000000,
    SIGBUS = 7, SIGSEGV = 11, PAGE = 4096, PAGES = 3, CASES = 4,
    RECORD_SIZE = 64, OUTPUT_SIZE = CASES * RECORD_SIZE, STORE_BASE = 512,
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

extern void latx_avx_single_vmaskmovdqu_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vmaskmovdqu_fault_store(uint8_t *, uint8_t *);
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

static void prepare(uint8_t *p)
{
    latx_avx_single_fill(p, PAGES * PAGE, UINT64_C(0x564d41534b4451));
    for (unsigned i = 0; i < 16; ++i) {
        p[192 + i] = (uint8_t)(0xa0 + i);
        p[128 + i] = 0;
        p[144 + i] = 0x80;
        p[160 + i] = (i & 1) ? 0 : 0x80;
        p[224 + i] = (uint8_t)(0x50 + i);
    }
    for (unsigned i = 0; i < 4; ++i)
        latx_avx_single_fill(p + STORE_BASE + i * 64, 64,
                             UINT64_C(0x53544f52454451) + i);
}

static int same(const char *left, const char *right)
{
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

int latx_avx_single_main(long argc, char **argv)
{
    long map = syscall6(SYS_MMAP, 0, PAGES * PAGE, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint8_t *page;

    if (map < 0)
        return 70;
    page = (uint8_t *)(uintptr_t)map;
    prepare(page);
    if (argc == 1) {
        latx_avx_single_vmaskmovdqu_run(output, page);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc != 2 || install_fault_handlers())
        return 72;
    if (syscall3(SYS_MPROTECT, (long)(uintptr_t)(page + PAGE), PAGE,
                 PROT_NONE) < 0)
        return 71;
    uint8_t *address = page + PAGE - 8;
    if (same(argv[1], "store-zero")) {
        latx_avx_single_vmaskmovdqu_fault_store(address, page + 128);
        return 0;
    }
    if (same(argv[1], "store-one")) {
        latx_avx_single_vmaskmovdqu_fault_store(address, page + 144);
        return 90;
    }
    if (same(argv[1], "store-mix")) {
        latx_avx_single_vmaskmovdqu_fault_store(address, page + 160);
        return 90;
    }
    return 74;
}
