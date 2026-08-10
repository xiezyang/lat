/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9, SYS_MPROTECT = 10, SYS_RT_SIGACTION = 13, SYS_EXIT = 60,
    PROT_NONE = 0, PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20, SA_SIGINFO = 4, SA_RESTORER = 0x04000000,
    SIGBUS = 7, SIGSEGV = 11, PAGE = 4096, PAGES = 2,
    CASES = 12, RECORD_SIZE = 32, OUTPUT_SIZE = CASES * RECORD_SIZE,
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

extern void latx_avx_single_vunpckhps_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vunpckhps_fault_xmm(uint8_t *);
extern void latx_avx_single_vunpckhps_fault_ymm(uint8_t *);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t output[OUTPUT_SIZE];
static uint8_t fault_capture[16];
static uint8_t *fault_target;
static const char *fault_name;

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

static inline long syscall6(long n, long a, long b, long c, long d,
                             long e, long f)
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

static int same(const char *left, const char *right)
{
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void copy_tag(void)
{
    for (unsigned i = 0; i < 8; ++i)
        fault_capture[i] = fault_name[i] ? (uint8_t)fault_name[i] : 0;
}

static void fault_handler(int sig, void *info, void *ctx)
{
    (void)info;
    (void)ctx;
    copy_tag();
    for (unsigned i = 0; i < 8; ++i)
        fault_capture[8 + i] = fault_target[i];
    latx_avx_single_write_all(fault_capture, sizeof(fault_capture));
    exit_now(128 + sig);
}

static int install_handlers(void)
{
    struct kernel_sigaction action = {
        fault_handler, SA_SIGINFO | SA_RESTORER, latx_avx_single_rt_sigreturn,
        0,
    };
    return syscall4(SYS_RT_SIGACTION, SIGSEGV, (long)(uintptr_t)&action, 0,
                    8) < 0 ||
           syscall4(SYS_RT_SIGACTION, SIGBUS, (long)(uintptr_t)&action, 0,
                    8) < 0;
}

int latx_avx_single_main(long argc, char **argv)
{
    long mapping;
    uint8_t *page;

    if (argc == 1) {
        latx_avx_single_vunpckhps_run(output, 0);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc != 2 || (!same(argv[1], "xmm-cross-8") &&
                      !same(argv[1], "ymm-cross-16")))
        return 2;
    mapping = syscall6(SYS_MMAP, 0, PAGES * PAGE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping < 0)
        return 3;
    page = (uint8_t *)(uintptr_t)mapping;
    latx_avx_single_fill(page, PAGES * PAGE, UINT64_C(0x91e10da4c3b2f587));
    if (install_handlers() ||
        syscall3(SYS_MPROTECT, (long)(uintptr_t)(page + PAGE), PAGE,
                 PROT_NONE) < 0)
        return 4;
    if (same(argv[1], "xmm-cross-8")) {
        fault_name = "xmm-cross-8";
        fault_target = page + PAGE - 8;
        latx_avx_single_vunpckhps_fault_xmm(fault_target);
    } else {
        fault_name = "ymm-cross-16";
        fault_target = page + PAGE - 16;
        latx_avx_single_vunpckhps_fault_ymm(fault_target);
    }
    return 5;
}
