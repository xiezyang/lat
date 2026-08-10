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
    LATX_VPOR_PAGES = 16,
    LATX_VPOR_CASES = 84,
    LATX_VPOR_OUTPUT_SIZE = LATX_VPOR_CASES * 32,
};

struct latx_kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct latx_vpor_fault_record {
    int32_t signal_number;
    int32_t signal_code;
    uint64_t fault_offset;
    uint8_t xmm15[16];
    uint8_t ymmh15[16];
};

extern void latx_avx_single_vpor_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vpor_logic(uint8_t *);
extern void latx_avx_single_vpor_fault_xmm(uint8_t *);
extern void latx_avx_single_vpor_fault_ymm(uint8_t *);
extern void latx_avx_single_vpor_fault_observe(void);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t vpor_output[LATX_VPOR_OUTPUT_SIZE];
static uint8_t vpor_logic_output[32];
static volatile uintptr_t vpor_fault_base;

static inline long latx_vpor_syscall4(long number, long arg0,
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

static inline long latx_vpor_syscall6(long number, long arg0,
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

static __attribute__((noreturn)) void latx_vpor_exit(int status)
{
    __asm__ volatile("syscall"
                     :
                     : "a"((long)LATX_SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void latx_vpor_copy(uint8_t *dest, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dest[i] = src[i];
    }
}

static void latx_vpor_fault_handler(int signal_number, void *siginfo,
                                     void *ucontext)
{
    const uint8_t *siginfo_bytes = siginfo;
    const uint8_t *ucontext_bytes = ucontext;
    uintptr_t fault_address = *(const uintptr_t *)(siginfo_bytes + 16);
    uintptr_t fpstate_address = *(const uintptr_t *)(ucontext_bytes + 224);
    struct latx_vpor_fault_record record = {0};

    latx_avx_single_vpor_fault_observe();
    record.signal_number = signal_number;
    record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
    record.fault_offset = fault_address - vpor_fault_base;
    if (fpstate_address != 0) {
        const uint8_t *fpstate = (const uint8_t *)fpstate_address;

        latx_vpor_copy(record.xmm15, fpstate + 160 + 15 * 16, 16);
        latx_vpor_copy(record.ymmh15, fpstate + 576 + 15 * 16, 16);
    }
    latx_avx_single_write_all(&record, sizeof(record));
    latx_vpor_exit(128 + signal_number);
}

static int latx_vpor_install_handler(int signal_number)
{
    struct latx_kernel_sigaction action = {
        .handler = latx_vpor_fault_handler,
        .flags = LATX_SA_SIGINFO | LATX_SA_RESTORER,
        .restorer = latx_avx_single_rt_sigreturn,
        .mask = 0,
    };

    return latx_vpor_syscall4(
        LATX_SYS_RT_SIGACTION, signal_number,
        (long)(uintptr_t)&action, 0, 8) < 0;
}

static int latx_vpor_streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void latx_vpor_fill_pattern(uint8_t *dest, int pattern)
{
    for (size_t i = 0; i < 32; ++i) {
        switch (pattern) {
        case 0:
            dest[i] = 0;
            break;
        case 1:
            dest[i] = 0xff;
            break;
        case 2:
            dest[i] = (i % 8 == 7) ? 0x80 : 0;
            break;
        case 3:
            dest[i] = (uint8_t)i;
            break;
        default:
            dest[i] = (i & 1) ? 0xaa : 0x55;
            break;
        }
    }
}

static void latx_vpor_prepare_memory(uint8_t *page)
{
    latx_avx_single_fill(page, LATX_VPOR_PAGES * LATX_PAGE_SIZE,
                         UINT64_C(0xb75019e32ad46c8f));
    latx_vpor_fill_pattern(page, 0);
    latx_vpor_fill_pattern(page + 8192 + 13, 1);
    latx_vpor_fill_pattern(page + 16384 + 120, 2);
    latx_vpor_fill_pattern(page + 25600, 3);
    latx_vpor_fill_pattern(page + 32768 + 1, 4);
}

static int latx_vpor_fault_case(const char *name, uint8_t *page)
{
    uint8_t *address = 0;
    void (*operation)(uint8_t *) = 0;

    if (latx_vpor_streq(name, "xmm-cross-1")) {
        address = page + 4081;
        operation = latx_avx_single_vpor_fault_xmm;
    } else if (latx_vpor_streq(name, "xmm-cross-15")) {
        address = page + 4095;
        operation = latx_avx_single_vpor_fault_xmm;
    } else if (latx_vpor_streq(name, "ymm-cross-1")) {
        address = page + 4065;
        operation = latx_avx_single_vpor_fault_ymm;
    } else if (latx_vpor_streq(name, "ymm-cross-15")) {
        address = page + 4079;
        operation = latx_avx_single_vpor_fault_ymm;
    } else if (latx_vpor_streq(name, "ymm-cross-16")) {
        address = page + 4080;
        operation = latx_avx_single_vpor_fault_ymm;
    } else if (latx_vpor_streq(name, "ymm-cross-31")) {
        address = page + 4095;
        operation = latx_avx_single_vpor_fault_ymm;
    } else {
        return 74;
    }

    vpor_fault_base = (uintptr_t)page;
    operation(address);
    return 90;
}

int latx_avx_single_main(long argc, char **argv)
{
    long mapping = latx_vpor_syscall6(
        LATX_SYS_MMAP, 0, LATX_VPOR_PAGES * LATX_PAGE_SIZE,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);
    uint8_t *page;

    if (mapping < 0) {
        return 70;
    }
    page = (uint8_t *)(uintptr_t)mapping;
    latx_vpor_prepare_memory(page);

    if (argc == 2 && latx_vpor_streq(argv[1], "logic")) {
        latx_avx_single_vpor_logic(vpor_logic_output);
        return latx_avx_single_write_all(vpor_logic_output,
                                         sizeof(vpor_logic_output)) != 0;
    }

    if (argc == 1 ||
        (argc == 2 && latx_vpor_streq(argv[1], "reference"))) {
        latx_avx_single_vpor_run(vpor_output, page);
        return latx_avx_single_write_all(vpor_output,
                                         sizeof(vpor_output)) != 0;
    }
    if (argc != 2) {
        return 72;
    }
    if (latx_vpor_streq(argv[1], "trace")) {
        latx_avx_single_vpor_run(0, page);
        return 0;
    }
    if (latx_vpor_install_handler(LATX_SIGSEGV) != 0 ||
        latx_vpor_install_handler(LATX_SIGBUS) != 0) {
        return 73;
    }
    if (latx_avx_single_syscall3(
            LATX_SYS_MPROTECT, (long)(uintptr_t)(page + LATX_PAGE_SIZE),
            LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
        return 71;
    }
    return latx_vpor_fault_case(argv[1], page);
}
