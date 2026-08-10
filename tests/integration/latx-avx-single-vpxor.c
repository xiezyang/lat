/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9,
    SYS_MPROTECT = 10,
    SYS_RT_SIGACTION = 13,
    SYS_EXIT = 60,
    PROT_READ = 1,
    PROT_WRITE = 2,
    PROT_NONE = 0,
    MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20,
    SA_SIGINFO = 4,
    SA_RESTORER = 0x04000000,
    SIGBUS = 7,
    SIGSEGV = 11,
    PAGE = 4096,
    PAGES = 2,
    VPXOR_CASES = 16,
    VPXOR_OUTPUT_SIZE = VPXOR_CASES * 32,
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct fault_record {
    int32_t signal_number;
    int32_t signal_code;
    uint64_t fault_offset;
};

extern void latx_avx_single_vpxor_run(uint8_t *output);
extern void latx_avx_single_vpxor_fault_xmm(uint8_t *address);
extern void latx_avx_single_vpxor_fault_ymm(uint8_t *address);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t vpxor_output[VPXOR_OUTPUT_SIZE];
static volatile uintptr_t fault_base;
static const char *fault_name;

static inline long syscall3(long number, long arg0, long arg1, long arg2)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = arg0;
    register long rsi __asm__("rsi") = arg1;
    register long rdx __asm__("rdx") = arg2;

    __asm__ volatile("syscall" : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx)
                     : "rcx", "r11", "memory");
    return rax;
}

static inline long syscall4(long number, long arg0, long arg1, long arg2,
                            long arg3)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = arg0;
    register long rsi __asm__("rsi") = arg1;
    register long rdx __asm__("rdx") = arg2;
    register long r10 __asm__("r10") = arg3;

    __asm__ volatile("syscall" : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10)
                     : "rcx", "r11", "memory");
    return rax;
}

static inline long syscall6(long number, long arg0, long arg1, long arg2,
                            long arg3, long arg4, long arg5)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = arg0;
    register long rsi __asm__("rsi") = arg1;
    register long rdx __asm__("rdx") = arg2;
    register long r10 __asm__("r10") = arg3;
    register long r8 __asm__("r8") = arg4;
    register long r9 __asm__("r9") = arg5;

    __asm__ volatile("syscall" : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10),
                       "r"(r8), "r"(r9)
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
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void fault_handler(int signal_number, void *siginfo, void *ucontext)
{
    const uint8_t *siginfo_bytes = siginfo;
    const uintptr_t fault_address = *(const uintptr_t *)(siginfo_bytes + 16);
    struct fault_record record = {
        .signal_number = signal_number,
        .signal_code = *(const int32_t *)(siginfo_bytes + 8),
        .fault_offset = fault_address - fault_base,
    };

    (void)ucontext;
    latx_avx_single_write_all(&record, sizeof(record));
    exit_now(128 + signal_number);
}

static int install_handlers(void)
{
    struct kernel_sigaction action = {
        .handler = fault_handler,
        .flags = SA_SIGINFO | SA_RESTORER,
        .restorer = latx_avx_single_rt_sigreturn,
        .mask = 0,
    };

    return syscall4(SYS_RT_SIGACTION, SIGSEGV,
                    (long)(uintptr_t)&action, 0, 8) < 0 ||
           syscall4(SYS_RT_SIGACTION, SIGBUS,
                    (long)(uintptr_t)&action, 0, 8) < 0;
}

int latx_avx_single_main(long argc, char **argv)
{
    long mapping;
    uint8_t *page;

    if (argc == 1) {
        latx_avx_single_vpxor_run(vpxor_output);
        return latx_avx_single_write_all(vpxor_output,
                                         sizeof(vpxor_output)) != 0;
    }
    if (argc != 2 || (!same(argv[1], "xmm-cross-8") &&
                      !same(argv[1], "ymm-cross-16"))) {
        return 2;
    }

    mapping = syscall6(SYS_MMAP, 0, PAGES * PAGE,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping < 0) {
        return 3;
    }
    page = (uint8_t *)(uintptr_t)mapping;
    if (install_handlers() ||
        syscall3(SYS_MPROTECT, (long)(uintptr_t)(page + PAGE), PAGE,
                 PROT_NONE) < 0) {
        return 4;
    }

    fault_base = (uintptr_t)page;
    if (same(argv[1], "xmm-cross-8")) {
        fault_name = "xmm-cross-8";
        latx_avx_single_vpxor_fault_xmm(page + PAGE - 8);
    } else {
        fault_name = "ymm-cross-16";
        latx_avx_single_vpxor_fault_ymm(page + PAGE - 16);
    }
    (void)fault_name;
    return 5;
}
