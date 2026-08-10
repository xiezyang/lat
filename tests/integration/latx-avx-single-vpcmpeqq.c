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
    LATX_VPCMPEQQ_PAGES = 16,
    LATX_VPCMPEQQ_CASES = 84,
    LATX_VPCMPEQQ_OUTPUT_SIZE = LATX_VPCMPEQQ_CASES * 32,
};

struct latx_kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct latx_vpcmpeqq_fault_record {
    int32_t signal_number;
    int32_t signal_code;
    uint64_t fault_offset;
    uint8_t xmm15[16];
    uint8_t ymmh15[16];
};

extern void latx_avx_single_vpcmpeqq_run(uint8_t *, uint8_t *, int);
extern void latx_avx_single_vpcmpeqq_fault_xmm(uint8_t *);
extern void latx_avx_single_vpcmpeqq_fault_ymm(uint8_t *);
extern void latx_avx_single_vpcmpeqq_fault_observe(void);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t vpcmpeqq_output[LATX_VPCMPEQQ_OUTPUT_SIZE];
static volatile uintptr_t vpcmpeqq_fault_base;

static inline long latx_vpcmpeqq_syscall4(long number, long arg0,
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

static inline long latx_vpcmpeqq_syscall6(long number, long arg0,
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

static __attribute__((noreturn)) void latx_vpcmpeqq_exit(int status)
{
    __asm__ volatile("syscall"
                     :
                     : "a"((long)LATX_SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void latx_vpcmpeqq_copy(uint8_t *dest, const uint8_t *src,
                               size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dest[i] = src[i];
    }
}

static void latx_vpcmpeqq_fault_handler(int signal_number, void *siginfo,
                                        void *ucontext)
{
    const uint8_t *siginfo_bytes = siginfo;
    const uint8_t *ucontext_bytes = ucontext;
    uintptr_t fault_address = *(const uintptr_t *)(siginfo_bytes + 16);
    uintptr_t fpstate_address = *(const uintptr_t *)(ucontext_bytes + 224);
    struct latx_vpcmpeqq_fault_record record = {0};

    latx_avx_single_vpcmpeqq_fault_observe();
    record.signal_number = signal_number;
    record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
    record.fault_offset = fault_address - vpcmpeqq_fault_base;
    if (fpstate_address != 0) {
        const uint8_t *fpstate = (const uint8_t *)fpstate_address;

        latx_vpcmpeqq_copy(record.xmm15,
                           fpstate + 160 + 15 * 16, 16);
        latx_vpcmpeqq_copy(record.ymmh15,
                           fpstate + 576 + 15 * 16, 16);
    }
    latx_avx_single_write_all(&record, sizeof(record));
    latx_vpcmpeqq_exit(128 + signal_number);
}

static int latx_vpcmpeqq_install_handler(int signal_number)
{
    struct latx_kernel_sigaction action = {
        .handler = latx_vpcmpeqq_fault_handler,
        .flags = LATX_SA_SIGINFO | LATX_SA_RESTORER,
        .restorer = latx_avx_single_rt_sigreturn,
        .mask = 0,
    };

    return latx_vpcmpeqq_syscall4(
        LATX_SYS_RT_SIGACTION, signal_number,
        (long)(uintptr_t)&action, 0, 8) < 0;
}

static int latx_vpcmpeqq_streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void latx_vpcmpeqq_store_u64(uint8_t *dest, uint64_t value)
{
    for (size_t i = 0; i < 8; ++i) {
        dest[i] = (uint8_t)(value >> (i * 8));
    }
}

static void latx_vpcmpeqq_store_pattern(uint8_t *dest, int pattern)
{
    static const uint64_t patterns[][4] = {
        {0, 0, 0, 0},
        {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX},
        {0, UINT64_MAX, 0, UINT64_MAX},
        {UINT64_MAX, 0, UINT64_MAX, 0},
        {UINT64_C(0x00000000ffffffff),
         UINT64_C(0xffffffff00000000), 0, UINT64_MAX},
        {UINT64_MAX, UINT64_MAX, 0, 0},
        {UINT64_C(0x00000000ffffffff), 0,
         UINT64_C(0xffffffff00000000), UINT64_MAX},
    };

    for (size_t i = 0; i < 4; ++i) {
        latx_vpcmpeqq_store_u64(dest + i * 8, patterns[pattern][i]);
    }
}

static void latx_vpcmpeqq_prepare_memory(uint8_t *page)
{
    latx_avx_single_fill(page, LATX_VPCMPEQQ_PAGES * LATX_PAGE_SIZE,
                         UINT64_C(0x24b7619d83e50acf));
    latx_vpcmpeqq_store_pattern(page, 0);
    latx_vpcmpeqq_store_pattern(page + 8192 + 13, 1);
    latx_vpcmpeqq_store_pattern(page + 16384 + 120, 2);
    latx_vpcmpeqq_store_pattern(page + 25600, 3);
    latx_vpcmpeqq_store_pattern(page + 32768 + 1, 4);
    latx_vpcmpeqq_store_pattern(page + 40960 + 4064, 5);
    latx_vpcmpeqq_store_pattern(page + 49152 + 4080, 6);
}

static int latx_vpcmpeqq_fault_case(const char *name, uint8_t *page)
{
    uint8_t *address = 0;
    void (*operation)(uint8_t *) = 0;

    if (latx_vpcmpeqq_streq(name, "xmm-cross-1")) {
        address = page + 4081;
        operation = latx_avx_single_vpcmpeqq_fault_xmm;
    } else if (latx_vpcmpeqq_streq(name, "xmm-cross-15")) {
        address = page + 4095;
        operation = latx_avx_single_vpcmpeqq_fault_xmm;
    } else if (latx_vpcmpeqq_streq(name, "ymm-cross-1")) {
        address = page + 4065;
        operation = latx_avx_single_vpcmpeqq_fault_ymm;
    } else if (latx_vpcmpeqq_streq(name, "ymm-cross-15")) {
        address = page + 4079;
        operation = latx_avx_single_vpcmpeqq_fault_ymm;
    } else if (latx_vpcmpeqq_streq(name, "ymm-cross-16")) {
        address = page + 4080;
        operation = latx_avx_single_vpcmpeqq_fault_ymm;
    } else if (latx_vpcmpeqq_streq(name, "ymm-cross-31")) {
        address = page + 4095;
        operation = latx_avx_single_vpcmpeqq_fault_ymm;
    } else {
        return 74;
    }

    vpcmpeqq_fault_base = (uintptr_t)page;
    operation(address);
    return 90;
}

int latx_avx_single_main(long argc, char **argv)
{
    long mapping = latx_vpcmpeqq_syscall6(
        LATX_SYS_MMAP, 0, LATX_VPCMPEQQ_PAGES * LATX_PAGE_SIZE,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);
    uint8_t *page;

    if (mapping < 0) {
        return 70;
    }
    page = (uint8_t *)(uintptr_t)mapping;
    latx_vpcmpeqq_prepare_memory(page);

    if (argc == 1 ||
        (argc == 2 && latx_vpcmpeqq_streq(argv[1], "reference"))) {
        latx_avx_single_vpcmpeqq_run(vpcmpeqq_output, page, 0);
        return latx_avx_single_write_all(vpcmpeqq_output,
                                         sizeof(vpcmpeqq_output)) != 0;
    }
    if (argc != 2) {
        return 72;
    }
    if (latx_vpcmpeqq_streq(argv[1], "trace0")) {
        latx_avx_single_vpcmpeqq_run(0, page, 1);
        return 0;
    }
    if (latx_vpcmpeqq_streq(argv[1], "trace15")) {
        latx_avx_single_vpcmpeqq_run(0, page, 2);
        return 0;
    }
    if (latx_vpcmpeqq_install_handler(LATX_SIGSEGV) != 0 ||
        latx_vpcmpeqq_install_handler(LATX_SIGBUS) != 0) {
        return 73;
    }
    if (latx_avx_single_syscall3(
            LATX_SYS_MPROTECT, (long)(uintptr_t)(page + LATX_PAGE_SIZE),
            LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
        return 71;
    }
    return latx_vpcmpeqq_fault_case(argv[1], page);
}
