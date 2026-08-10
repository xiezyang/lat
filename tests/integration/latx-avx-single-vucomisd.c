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
    LATX_SIGFPE = 8,
    LATX_SIGSEGV = 11,
    LATX_PAGE_SIZE = 4096,
    LATX_VUCOMISD_PAGES = 16,
    LATX_VUCOMISD_CASES = 84,
    LATX_VUCOMISD_RECORD_SIZE = 32,
    LATX_VUCOMISD_OUTPUT_SIZE =
        LATX_VUCOMISD_CASES * LATX_VUCOMISD_RECORD_SIZE,
};

struct latx_kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct latx_vucomisd_fault_record {
    int32_t signal_number;
    int32_t signal_code;
    uint64_t fault_offset;
    uint8_t xmm15[16];
    uint32_t mxcsr;
    uint32_t reserved0;
    uint64_t rflags;
    uint8_t reserved1[16];
};

struct latx_vucomisd_fpe_record {
    int32_t signal_number;
    int32_t signal_code;
    int64_t signal_address_offset;
    int64_t rip_offset;
    uint16_t control_word;
    uint16_t status_word;
    uint32_t mxcsr;
    uint64_t rflags;
    uint8_t xmm15[16];
    uint8_t reserved[8];
};

extern uint8_t latx_avx_vucomisd_rip_source[8];
extern void latx_avx_single_vucomisd_run(uint8_t *, uint8_t *, int, int);
extern void latx_avx_single_vucomisd_fault(uint8_t *, int);
extern void latx_avx_single_vucomisd_fault_unmasked(uint8_t *, int);
extern void latx_avx_single_vucomisd_unmasked(uint8_t *, int, int);
extern uint8_t latx_avx_single_vucomisd_unmasked_site[];
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t vucomisd_output[LATX_VUCOMISD_OUTPUT_SIZE];
static volatile uintptr_t vucomisd_fault_base;
static volatile int vucomisd_expect_fpe;

static inline long latx_vucomisd_syscall4(long number, long arg0,
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

static inline long latx_vucomisd_syscall6(long number, long arg0,
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

static __attribute__((noreturn)) void latx_vucomisd_exit(int status)
{
    __asm__ volatile("syscall"
                     :
                     : "a"((long)LATX_SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void latx_vucomisd_copy(uint8_t *dest, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dest[i] = src[i];
    }
}

static int latx_vucomisd_streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static uint8_t *latx_vucomisd_map(void)
{
    long mapping = latx_vucomisd_syscall6(
        LATX_SYS_MMAP, 0, LATX_VUCOMISD_PAGES * LATX_PAGE_SIZE,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);

    if ((unsigned long)mapping >= (unsigned long)-4095) {
        return 0;
    }
    return (uint8_t *)(uintptr_t)mapping;
}

static void latx_vucomisd_store_u64(uint8_t *dest, uint64_t value)
{
    for (size_t i = 0; i < 8; ++i) {
        dest[i] = (uint8_t)(value >> (i * 8));
    }
}

static void latx_vucomisd_prepare_memory(uint8_t *page)
{
    latx_avx_single_fill(page, LATX_VUCOMISD_PAGES * LATX_PAGE_SIZE,
                         UINT64_C(0x5655434f4d495344));
    latx_vucomisd_store_u64(page, UINT64_C(0x3ff0000000000000));
    latx_vucomisd_store_u64(page + 8192 + 13,
                            UINT64_C(0xc000000000000000));
    latx_vucomisd_store_u64(page + 16384 + 120,
                            UINT64_C(0x0000000000000000));
    latx_vucomisd_store_u64(page + 25600,
                            UINT64_C(0x7ff8000000001234));
    latx_vucomisd_store_u64(page + 32768 + 1,
                            UINT64_C(0x7ff0000000001234));
    latx_vucomisd_store_u64(page + 40960 + 4088,
                            UINT64_C(0x0000000000000001));
    latx_vucomisd_store_u64(page + 49152 + 4092,
                            UINT64_C(0x7ff0000000000000));
    latx_vucomisd_store_u64(latx_avx_vucomisd_rip_source,
                            UINT64_C(0xfff0000000000000));
}

static void latx_vucomisd_signal_handler(int signal_number, void *siginfo,
                                         void *ucontext)
{
    const uint8_t *siginfo_bytes = siginfo;
    const uint8_t *ucontext_bytes = ucontext;
    uintptr_t signal_address = *(const uintptr_t *)(siginfo_bytes + 16);
    uintptr_t fpstate_address = *(const uintptr_t *)(ucontext_bytes + 224);

    if (vucomisd_expect_fpe) {
        struct latx_vucomisd_fpe_record record = {0};
        uintptr_t rip = *(const uintptr_t *)(ucontext_bytes + 168);

        record.signal_number = signal_number;
        record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
        record.signal_address_offset =
            (int64_t)(signal_address -
                      (uintptr_t)latx_avx_single_vucomisd_unmasked_site);
        record.rip_offset =
            (int64_t)(rip -
                      (uintptr_t)latx_avx_single_vucomisd_unmasked_site);
        record.rflags = *(const uint64_t *)(ucontext_bytes + 176);
        if (fpstate_address != 0) {
            const uint8_t *fpstate = (const uint8_t *)fpstate_address;

            record.control_word = *(const uint16_t *)(fpstate + 0);
            record.status_word = *(const uint16_t *)(fpstate + 2);
            record.mxcsr = *(const uint32_t *)(fpstate + 24);
            latx_vucomisd_copy(
                record.xmm15, fpstate + 160 + 15 * 16, 16);
        }
        latx_avx_single_write_all(&record, sizeof(record));
        latx_vucomisd_exit(128 + signal_number);
    } else {
        struct latx_vucomisd_fault_record record = {0};

        record.signal_number = signal_number;
        record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
        record.fault_offset = signal_address - vucomisd_fault_base;
        record.rflags = *(const uint64_t *)(ucontext_bytes + 176);
        if (fpstate_address != 0) {
            const uint8_t *fpstate = (const uint8_t *)fpstate_address;

            latx_vucomisd_copy(
                record.xmm15, fpstate + 160 + 15 * 16, 16);
            record.mxcsr = *(const uint32_t *)(fpstate + 24);
        }
        latx_avx_single_write_all(&record, sizeof(record));
        latx_vucomisd_exit(128 + signal_number);
    }
}

static int latx_vucomisd_install_handler(int signal_number)
{
    struct latx_kernel_sigaction action = {
        .handler = latx_vucomisd_signal_handler,
        .flags = LATX_SA_SIGINFO | LATX_SA_RESTORER,
        .restorer = latx_avx_single_rt_sigreturn,
        .mask = 0,
    };

    return latx_vucomisd_syscall4(
        LATX_SYS_RT_SIGACTION, signal_number,
        (long)(uintptr_t)&action, 0, 8) < 0;
}

static int latx_vucomisd_fault_case(const char *name, uint8_t *page)
{
    uint8_t *address;

    if (latx_vucomisd_streq(name, "load-cross-1")) {
        address = page + 4089;
    } else if (latx_vucomisd_streq(name, "load-cross-7") ||
               latx_vucomisd_streq(name, "load-cross-unmasked")) {
        address = page + 4095;
    } else {
        return 74;
    }
    if (latx_avx_single_syscall3(
            LATX_SYS_MPROTECT,
            (long)(uintptr_t)(page + LATX_PAGE_SIZE),
            LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
        return 71;
    }
    vucomisd_fault_base = (uintptr_t)page;
    if (latx_vucomisd_streq(name, "load-cross-unmasked")) {
        latx_avx_single_vucomisd_fault_unmasked(address, 0);
    } else {
        latx_avx_single_vucomisd_fault(address, 0);
    }
    return 90;
}

static int latx_vucomisd_exception_mode(const char *name)
{
    static const char *const names[] = {
        "fpe-invalid",
        "fpe-denormal",
        "fpe-invalid-priority",
        "no-signal-qnan-suppresses-denormal",
        "no-signal-daz",
        "no-signal-old-sticky",
        "no-signal-invalid-suppresses-denormal",
    };

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        if (latx_vucomisd_streq(name, names[i])) {
            return (int)i + 1;
        }
    }
    return -1;
}

int latx_avx_single_main(long argc, char **argv)
{
    uint8_t *page = latx_vucomisd_map();
    int mode;

    if (page == 0) {
        return 70;
    }
    latx_vucomisd_prepare_memory(page);

    if (argc == 1 ||
        (argc == 2 && latx_vucomisd_streq(argv[1], "reference")) ||
        (argc == 2 && latx_vucomisd_streq(argv[1], "actual"))) {
        latx_avx_single_vucomisd_run(vucomisd_output, page, 0, 0);
        return latx_avx_single_write_all(
                   vucomisd_output, sizeof(vucomisd_output)) != 0;
    }
    if (argc != 2) {
        return 72;
    }
    mode = latx_vucomisd_exception_mode(argv[1]);
    if (mode >= 0) {
        if (mode <= 3) {
            vucomisd_expect_fpe = 1;
            if (latx_vucomisd_install_handler(LATX_SIGFPE) != 0) {
                return 73;
            }
        }
        latx_avx_single_vucomisd_unmasked(
            mode <= 3 ? 0 : vucomisd_output, mode, 0);
        if (mode <= 3) {
            return 91;
        }
        return latx_avx_single_write_all(
                   vucomisd_output, LATX_VUCOMISD_RECORD_SIZE) != 0;
    }
    if (latx_vucomisd_install_handler(LATX_SIGSEGV) != 0 ||
        latx_vucomisd_install_handler(LATX_SIGBUS) != 0) {
        return 73;
    }
    return latx_vucomisd_fault_case(argv[1], page);
}
