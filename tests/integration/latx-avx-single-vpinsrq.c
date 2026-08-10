/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    LATX_SYS_MMAP = 9,
    LATX_SYS_MPROTECT = 10,
    LATX_SYS_RT_SIGACTION = 13,
    LATX_SYS_EXIT = 60,
    LATX_PROT_NONE = 0,
    LATX_PROT_READ = 1,
    LATX_PROT_WRITE = 2,
    LATX_MAP_PRIVATE = 2,
    LATX_MAP_ANONYMOUS = 0x20,
    LATX_SA_SIGINFO = 4,
    LATX_SA_RESTORER = 0x04000000,
    LATX_SIGBUS = 7,
    LATX_SIGSEGV = 11,
    LATX_PAGE_SIZE = 4096,
    LATX_VPINSRQ_OUTPUT_SIZE = 8384,
};

struct latx_kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct latx_vpinsrq_fault_record {
    int32_t signal_number;
    int32_t signal_code;
    uint64_t fault_offset;
    uint8_t xmm15[16];
};

extern void latx_avx_single_vpinsrq_run(uint8_t *output, uint8_t *page);
extern void latx_avx_single_vpinsrq_trace(void);
extern void latx_avx_single_vpinsrq_fault_invalid(void);
extern void latx_avx_single_vpinsrq_fault_cross_page(uint8_t *page);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t vpinsrq_output[LATX_VPINSRQ_OUTPUT_SIZE];
static volatile uintptr_t vpinsrq_fault_base;

static inline long latx_avx_single_syscall4(long number, long arg0,
                                            long arg1, long arg2, long arg3)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = arg0;
    register long rsi __asm__("rsi") = arg1;
    register long rdx __asm__("rdx") = arg2;
    register long r10 __asm__("r10") = arg3;

    __asm__ volatile("syscall"
                     : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10)
                     : "rcx", "r11", "memory");
    return rax;
}

static inline long latx_avx_single_syscall6(long number, long arg0,
                                            long arg1, long arg2, long arg3,
                                            long arg4, long arg5)
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

static __attribute__((noreturn)) void latx_vpinsrq_exit(int status)
{
    __asm__ volatile("syscall"
                     :
                     : "a"((long)LATX_SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void latx_vpinsrq_copy(uint8_t *dest, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dest[i] = src[i];
    }
}

static void latx_vpinsrq_fault_handler(int signal_number, void *siginfo,
                                       void *ucontext)
{
    const uint8_t *siginfo_bytes = siginfo;
    const uint8_t *ucontext_bytes = ucontext;
    const uint8_t *fpstate;
    uintptr_t fault_address;
    uintptr_t fpstate_address;
    struct latx_vpinsrq_fault_record record = {0};

    record.signal_number = signal_number;
    record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
    fault_address = *(const uintptr_t *)(siginfo_bytes + 16);
    record.fault_offset = fault_address - vpinsrq_fault_base;

    fpstate_address = *(const uintptr_t *)(ucontext_bytes + 224);
    fpstate = (const uint8_t *)fpstate_address;
    if (fpstate != NULL) {
        latx_vpinsrq_copy(record.xmm15, fpstate + 160 + 15 * 16,
                          sizeof(record.xmm15));
    }

    latx_avx_single_write_all(&record, sizeof(record));
    latx_vpinsrq_exit(128 + signal_number);
}

static int latx_vpinsrq_install_handler(int signal_number)
{
    struct latx_kernel_sigaction action = {
        .handler = latx_vpinsrq_fault_handler,
        .flags = LATX_SA_SIGINFO | LATX_SA_RESTORER,
        .restorer = latx_avx_single_rt_sigreturn,
        .mask = 0,
    };

    return latx_avx_single_syscall4(LATX_SYS_RT_SIGACTION, signal_number,
                                     (long)(uintptr_t)&action, 0, 8) < 0;
}

static int latx_vpinsrq_streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

int latx_avx_single_main(long argc, char **argv)
{
    uint8_t *page;
    long mapping;

    mapping = latx_avx_single_syscall6(
        LATX_SYS_MMAP, 0, 2 * LATX_PAGE_SIZE,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);
    if (mapping < 0) {
        return 70;
    }
    page = (uint8_t *)(uintptr_t)mapping;
    latx_avx_single_fill(page, LATX_PAGE_SIZE,
                         UINT64_C(0x34de7921a5c80fb6));
    if (latx_avx_single_syscall3(LATX_SYS_MPROTECT,
                                 (long)(uintptr_t)(page + LATX_PAGE_SIZE),
                                 LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
        return 71;
    }

    if (argc == 1) {
        latx_avx_single_vpinsrq_run(vpinsrq_output, page);
        return latx_avx_single_write_all(vpinsrq_output,
                                         sizeof(vpinsrq_output)) != 0;
    }
    if (argc != 2) {
        return 72;
    }
    if (latx_vpinsrq_streq(argv[1], "trace")) {
        latx_avx_single_vpinsrq_trace();
        return 0;
    }
    if (latx_vpinsrq_install_handler(LATX_SIGSEGV) != 0 ||
        latx_vpinsrq_install_handler(LATX_SIGBUS) != 0) {
        return 73;
    }
    if (latx_vpinsrq_streq(argv[1], "fault-invalid")) {
        vpinsrq_fault_base = 0;
        latx_avx_single_vpinsrq_fault_invalid();
        return 90;
    }
    if (latx_vpinsrq_streq(argv[1], "fault-cross-page")) {
        vpinsrq_fault_base = (uintptr_t)page;
        latx_avx_single_vpinsrq_fault_cross_page(page);
        return 91;
    }
    return 74;
}
