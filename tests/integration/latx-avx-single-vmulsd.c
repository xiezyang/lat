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
    LATX_VMULSD_PAGES = 16,
    LATX_VMULSD_CASES = 104,
    LATX_VMULSD_RECORD_SIZE = 64,
    LATX_VMULSD_OUTPUT_SIZE =
        LATX_VMULSD_CASES * LATX_VMULSD_RECORD_SIZE,
};

struct latx_kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct latx_vmulsd_fault_record {
    int32_t signal_number;
    int32_t signal_code;
    uint64_t fault_offset;
    uint8_t xmm15[16];
    uint8_t ymmh15[16];
    uint32_t mxcsr;
    uint8_t reserved[12];
};

struct latx_vmulsd_fpe_record {
    int32_t signal_number;
    int32_t signal_code;
    int64_t signal_address_offset;
    int64_t rip_offset;
    uint16_t control_word;
    uint16_t status_word;
    uint32_t mxcsr;
    uint8_t xmm15[16];
    uint8_t ymmh15[16];
};

extern uint8_t latx_avx_vmulsd_rip_source[8];
extern void latx_avx_single_vmulsd_run(uint8_t *, uint8_t *, int, int);
extern void latx_avx_single_vmulsd_fault(uint8_t *, int);
extern void latx_avx_single_vmulsd_fault_unmasked(uint8_t *, int);
extern void latx_avx_single_vmulsd_unmasked(uint8_t *, int, int);
extern void latx_avx_single_vmulsd_fault_observe(void);
extern uint8_t latx_avx_single_vmulsd_unmasked_site[];
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t vmulsd_output[LATX_VMULSD_OUTPUT_SIZE];
static volatile uintptr_t vmulsd_fault_base;
static volatile int vmulsd_expect_fpe;

static inline long latx_vmulsd_syscall4(long number, long arg0,
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

static inline long latx_vmulsd_syscall6(long number, long arg0,
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

static __attribute__((noreturn)) void latx_vmulsd_exit(int status)
{
    __asm__ volatile("syscall"
                     :
                     : "a"((long)LATX_SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void latx_vmulsd_copy(uint8_t *dest, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dest[i] = src[i];
    }
}

static int latx_vmulsd_streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static uint8_t *latx_vmulsd_map(void)
{
    long mapping = latx_vmulsd_syscall6(
        LATX_SYS_MMAP, 0, LATX_VMULSD_PAGES * LATX_PAGE_SIZE,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);

    if ((unsigned long)mapping >= (unsigned long)-4095) {
        return 0;
    }
    return (uint8_t *)(uintptr_t)mapping;
}

static void latx_vmulsd_store_u64(uint8_t *dest, uint64_t value)
{
    for (size_t i = 0; i < 8; ++i) {
        dest[i] = (uint8_t)(value >> (i * 8));
    }
}

static void latx_vmulsd_prepare_memory(uint8_t *page)
{
    latx_avx_single_fill(page, LATX_VMULSD_PAGES * LATX_PAGE_SIZE,
                         UINT64_C(0x564d554c5344cafe));
    latx_vmulsd_store_u64(page, UINT64_C(0x0000000000000000));
    latx_vmulsd_store_u64(page + 8192 + 13,
                          UINT64_C(0x8000000000000000));
    latx_vmulsd_store_u64(page + 16384 + 120,
                          UINT64_C(0x3ff8000000000000));
    latx_vmulsd_store_u64(page + 25600,
                          UINT64_C(0x7fefffffffffffff));
    latx_vmulsd_store_u64(page + 32768 + 1,
                          UINT64_C(0x7ff0000000001234));
    latx_vmulsd_store_u64(page + 40960 + 4088,
                          UINT64_C(0x0000000000000001));
    latx_vmulsd_store_u64(page + 49152 + 4092,
                          UINT64_C(0x7ff0000000000000));
    latx_vmulsd_store_u64(latx_avx_vmulsd_rip_source,
                          UINT64_C(0x7ff8000000005678));
}

static void latx_vmulsd_fault_handler(int signal_number, void *siginfo,
                                      void *ucontext)
{
    const uint8_t *siginfo_bytes = siginfo;
    const uint8_t *ucontext_bytes = ucontext;
    uintptr_t fault_address = *(const uintptr_t *)(siginfo_bytes + 16);
    uintptr_t fpstate_address = *(const uintptr_t *)(ucontext_bytes + 224);
    struct latx_vmulsd_fault_record record = {0};

    if (vmulsd_expect_fpe) {
        uintptr_t rip = *(const uintptr_t *)(ucontext_bytes + 168);
        struct latx_vmulsd_fpe_record fpe_record = {0};

        fpe_record.signal_number = signal_number;
        fpe_record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
        fpe_record.signal_address_offset =
            (int64_t)(fault_address -
                      (uintptr_t)latx_avx_single_vmulsd_unmasked_site);
        fpe_record.rip_offset =
            (int64_t)(rip -
                      (uintptr_t)latx_avx_single_vmulsd_unmasked_site);
        if (fpstate_address != 0) {
            const uint8_t *fpstate = (const uint8_t *)fpstate_address;

            fpe_record.control_word = *(const uint16_t *)(fpstate + 0);
            fpe_record.status_word = *(const uint16_t *)(fpstate + 2);
            fpe_record.mxcsr = *(const uint32_t *)(fpstate + 24);
            latx_vmulsd_copy(
                fpe_record.xmm15, fpstate + 160 + 15 * 16, 16);
            if (*(const uint32_t *)(fpstate + 464) == UINT32_C(0x46505853)) {
                latx_vmulsd_copy(
                    fpe_record.ymmh15, fpstate + 576 + 15 * 16, 16);
            }
        }
        latx_avx_single_write_all(&fpe_record, sizeof(fpe_record));
        latx_vmulsd_exit(128 + signal_number);
    }

    latx_avx_single_vmulsd_fault_observe();
    record.signal_number = signal_number;
    record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
    record.fault_offset = fault_address - vmulsd_fault_base;
    if (fpstate_address != 0) {
        const uint8_t *fpstate = (const uint8_t *)fpstate_address;

        latx_vmulsd_copy(record.xmm15, fpstate + 160 + 15 * 16, 16);
        latx_vmulsd_copy(record.ymmh15, fpstate + 576 + 15 * 16, 16);
        record.mxcsr = *(const uint32_t *)(fpstate + 24);
    }
    latx_avx_single_write_all(&record, sizeof(record));
    latx_vmulsd_exit(128 + signal_number);
}

static int latx_vmulsd_install_handler(int signal_number)
{
    struct latx_kernel_sigaction action = {
        .handler = latx_vmulsd_fault_handler,
        .flags = LATX_SA_SIGINFO | LATX_SA_RESTORER,
        .restorer = latx_avx_single_rt_sigreturn,
        .mask = 0,
    };

    return latx_vmulsd_syscall4(
        LATX_SYS_RT_SIGACTION, signal_number,
        (long)(uintptr_t)&action, 0, 8) < 0;
}

static int latx_vmulsd_fault_case(const char *name, uint8_t *page)
{
    uint8_t *address;
    int restore = 1;

    if (latx_vmulsd_streq(name, "load-cross-1") ||
        latx_vmulsd_streq(name, "trace-load-cross-1")) {
        address = page + 4089;
    } else if (latx_vmulsd_streq(name, "load-cross-7") ||
               latx_vmulsd_streq(name, "trace-load-cross-7")) {
        address = page + 4095;
    } else if (latx_vmulsd_streq(name, "load-cross-unmasked") ||
               latx_vmulsd_streq(name,
                                  "trace-load-cross-unmasked")) {
        address = page + 4095;
    } else {
        return 74;
    }
    if (name[0] == 't') {
        restore = 0;
    }
    if (latx_avx_single_syscall3(
            LATX_SYS_MPROTECT,
            (long)(uintptr_t)(page + LATX_PAGE_SIZE),
            LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
        return 71;
    }
    vmulsd_fault_base = (uintptr_t)page;
    if (latx_vmulsd_streq(name, "load-cross-unmasked") ||
        latx_vmulsd_streq(name, "trace-load-cross-unmasked")) {
        latx_avx_single_vmulsd_fault_unmasked(address, restore);
    } else {
        latx_avx_single_vmulsd_fault(address, restore);
    }
    return 90;
}

static int latx_vmulsd_unmasked_mode(const char *name, int *restore)
{
    static const char *const names[] = {
        "fpe-invalid",
        "fpe-denormal",
        "fpe-overflow",
        "fpe-underflow",
        "fpe-precision",
        "fpe-overflow-precision",
        "fpe-underflow-precision",
        "fpe-denormal-priority",
        "fpe-invalid-priority",
        "fpe-underflow-ftz",
        "no-signal-daz",
        "no-signal-old-sticky",
    };
    const char *mode_name = name;

    *restore = 1;
    if (name[0] == 't' && name[1] == 'r' && name[2] == 'a' &&
        name[3] == 'c' && name[4] == 'e' && name[5] == '-') {
        mode_name = name + 6;
        *restore = 0;
    }
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        if (latx_vmulsd_streq(mode_name, names[i])) {
            return (int)i + 1;
        }
    }
    return -1;
}

static int latx_vmulsd_mode(const char *name)
{
    if (latx_vmulsd_streq(name, "trace0-register")) {
        return 1;
    }
    if (latx_vmulsd_streq(name, "trace0-memory")) {
        return 2;
    }
    if (latx_vmulsd_streq(name, "trace15-register")) {
        return 3;
    }
    if (latx_vmulsd_streq(name, "trace15-memory")) {
        return 4;
    }
    return -1;
}

int latx_avx_single_main(long argc, char **argv)
{
    uint8_t *page = latx_vmulsd_map();
    int mode;
    int restore;

    if (page == 0) {
        return 70;
    }
    latx_vmulsd_prepare_memory(page);

    if (argc == 1 ||
        (argc == 2 && latx_vmulsd_streq(argv[1], "reference")) ||
        (argc == 2 && latx_vmulsd_streq(argv[1], "actual"))) {
        int restore = argc == 1 ||
            latx_vmulsd_streq(argv[1], "reference");

        if (latx_avx_single_syscall3(
                LATX_SYS_MPROTECT,
                (long)(uintptr_t)(page + 11 * LATX_PAGE_SIZE),
                LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
            return 71;
        }
        latx_avx_single_vmulsd_run(vmulsd_output, page, 0, restore);
        return latx_avx_single_write_all(
                   vmulsd_output, sizeof(vmulsd_output)) != 0;
    }
    if (argc != 2) {
        return 72;
    }
    mode = latx_vmulsd_unmasked_mode(argv[1], &restore);
    if (mode >= 0) {
        if (mode <= 10) {
            vmulsd_expect_fpe = 1;
            if (latx_vmulsd_install_handler(LATX_SIGFPE) != 0) {
                return 73;
            }
        }
        latx_avx_single_vmulsd_unmasked(
            mode <= 10 ? 0 : vmulsd_output, mode, restore);
        if (mode <= 10) {
            return 91;
        }
        return latx_avx_single_write_all(
                   vmulsd_output, LATX_VMULSD_RECORD_SIZE) != 0;
    }
    mode = latx_vmulsd_mode(argv[1]);
    if (mode >= 0) {
        if (latx_avx_single_syscall3(
                LATX_SYS_MPROTECT,
                (long)(uintptr_t)(page + 11 * LATX_PAGE_SIZE),
                LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
            return 71;
        }
        latx_avx_single_vmulsd_run(0, page, mode, 0);
        return 0;
    }
    if (latx_vmulsd_install_handler(LATX_SIGSEGV) != 0 ||
        latx_vmulsd_install_handler(LATX_SIGBUS) != 0) {
        return 73;
    }
    return latx_vmulsd_fault_case(argv[1], page);
}
