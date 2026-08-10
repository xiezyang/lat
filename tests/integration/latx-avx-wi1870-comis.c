/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9,
    SYS_RT_SIGACTION = 13,
    SYS_EXIT = 60,
    PROT_READ = 1,
    PROT_WRITE = 2,
    MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20,
    SA_SIGINFO = 4,
    SA_RESTORER = 0x04000000,
    SIGFPE = 8,
    PAGE = 4096,
    PAGES = 4,
    NORMAL_CASES = 9,
    RECORD_SIZE = 32,
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct fpe_record {
    int32_t signal_number;
    int32_t signal_code;
    int64_t signal_address_offset;
    int64_t rip_offset;
    uint32_t mxcsr;
    uint32_t reserved;
    uint8_t xmm15[16];
};

extern void latx_avx_wi1870_comis_run(uint8_t *, uint8_t *);
extern void latx_avx_wi1870_comis_unmasked(int);
extern uint8_t latx_avx_wi1870_comis_unmasked_site[];
extern void latx_avx_wi1870_rt_sigreturn(void);

static uint8_t output[NORMAL_CASES * RECORD_SIZE];
static volatile int expect_fpe;

static inline long syscall4(long number, long arg0, long arg1,
                            long arg2, long arg3)
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

static int equal(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void store64(uint8_t *dest, uint64_t value)
{
    for (unsigned i = 0; i < 8; ++i) {
        dest[i] = (uint8_t)(value >> (i * 8));
    }
}

static void prepare(uint8_t *page)
{
    latx_avx_single_fill(page, PAGES * PAGE, UINT64_C(0x434f4d49535749));
    store64(page, UINT64_C(0x3ff0000000000000));
    store64(page + 4090, UINT64_C(0x7ff8000000001234));
    store64(page + 8193, UINT64_C(0x0000000000000001));
    store64(page + 12287, UINT64_C(0x7ff0000000000000));
}

static void signal_handler(int signal_number, void *siginfo, void *ucontext)
{
    const uint8_t *si = siginfo;
    const uint8_t *uc = ucontext;
    uintptr_t fpstate_address = *(const uintptr_t *)(uc + 224);
    uintptr_t signal_address = *(const uintptr_t *)(si + 16);

    if (expect_fpe) {
        struct fpe_record record = {0};
        uintptr_t rip = *(const uintptr_t *)(uc + 168);

        record.signal_number = signal_number;
        record.signal_code = *(const int32_t *)(si + 8);
        record.signal_address_offset =
            (int64_t)(signal_address -
                      (uintptr_t)latx_avx_wi1870_comis_unmasked_site);
        record.rip_offset =
            (int64_t)(rip -
                      (uintptr_t)latx_avx_wi1870_comis_unmasked_site);
        if (fpstate_address != 0) {
            const uint8_t *fpstate = (const uint8_t *)fpstate_address;
            record.mxcsr = *(const uint32_t *)(fpstate + 24);
            for (unsigned i = 0; i < 16; ++i) {
                record.xmm15[i] = fpstate[160 + 15 * 16 + i];
            }
        }
        latx_avx_single_write_all(&record, sizeof(record));
        exit_now(128 + signal_number);
    }
    exit_now(128 + signal_number);
}

static int install_fpe_handler(void)
{
    struct kernel_sigaction action = {
        .handler = signal_handler,
        .flags = SA_SIGINFO | SA_RESTORER,
        .restorer = latx_avx_wi1870_rt_sigreturn,
        .mask = 0,
    };
    return syscall4(SYS_RT_SIGACTION, SIGFPE, (long)(uintptr_t)&action,
                    0, 8) < 0;
}

int latx_avx_single_main(long argc, char **argv)
{
    long mapping = syscall6(SYS_MMAP, 0, PAGES * PAGE,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint8_t *page;

    if (mapping < 0 || argc != 2) {
        return 70;
    }
    page = (uint8_t *)(uintptr_t)mapping;
    prepare(page);
    if (equal(argv[1], "reference")) {
        latx_avx_wi1870_comis_run(output, page);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (!equal(argv[1], "fpe-invalid") &&
        !equal(argv[1], "fpe-denormal")) {
        return 72;
    }
    if (install_fpe_handler()) {
        return 73;
    }
    expect_fpe = 1;
    latx_avx_wi1870_comis_unmasked(equal(argv[1], "fpe-invalid") ? 1 : 2);
    return 90;
}
