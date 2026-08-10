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
    LATX_VMOVSD_PAGES = 16,
    LATX_VMOVSD_CASES = 42,
    LATX_VMOVSD_RECORD_SIZE = 64,
    LATX_VMOVSD_OUTPUT_SIZE =
        LATX_VMOVSD_CASES * LATX_VMOVSD_RECORD_SIZE,
};

struct latx_kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct latx_vmovsd_fault_record {
    int32_t signal_number;
    int32_t signal_code;
    uint64_t fault_offset;
    uint8_t xmm15[16];
    uint8_t ymmh15[16];
    uint8_t memory[16];
};

extern uint8_t latx_avx_vmovsd_rip_source[8];
extern uint8_t latx_avx_vmovsd_rip_store[32];
extern void latx_avx_single_vmovsd_run(uint8_t *, uint8_t *, uint8_t *, int);
extern void latx_avx_single_vmovsd_fault_load(uint8_t *, int);
extern void latx_avx_single_vmovsd_fault_store(uint8_t *, int);
extern void latx_avx_single_vmovsd_fault_observe(void);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t vmovsd_output[LATX_VMOVSD_OUTPUT_SIZE];
static volatile uintptr_t vmovsd_fault_base;
static volatile uintptr_t vmovsd_fault_snapshot;

static inline long latx_vmovsd_syscall4(long number, long arg0,
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

static inline long latx_vmovsd_syscall6(long number, long arg0,
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

static __attribute__((noreturn)) void latx_vmovsd_exit(int status)
{
    __asm__ volatile("syscall"
                     :
                     : "a"((long)LATX_SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void latx_vmovsd_copy(uint8_t *dest, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dest[i] = src[i];
    }
}

static int latx_vmovsd_streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static uint8_t *latx_vmovsd_map(void)
{
    long mapping = latx_vmovsd_syscall6(
        LATX_SYS_MMAP, 0, LATX_VMOVSD_PAGES * LATX_PAGE_SIZE,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);

    if ((unsigned long)mapping >= (unsigned long)-4095) {
        return 0;
    }
    return (uint8_t *)(uintptr_t)mapping;
}

static void latx_vmovsd_store_u64(uint8_t *dest, uint64_t value)
{
    for (size_t i = 0; i < 8; ++i) {
        dest[i] = (uint8_t)(value >> (i * 8));
    }
}

static void latx_vmovsd_prepare_load_memory(uint8_t *page)
{
    static const uint64_t values[] = {
        0,
        UINT64_MAX,
        UINT64_C(0x8000000000000000),
        UINT64_C(0x0123456789abcdef),
        UINT64_C(0xfedcba9876543210),
        UINT64_C(0xaa55aa55aa55aa55),
        UINT64_C(0x0706050403020100),
    };

    latx_avx_single_fill(page, LATX_VMOVSD_PAGES * LATX_PAGE_SIZE,
                         UINT64_C(0x564d4f565344cafe));
    latx_vmovsd_store_u64(page, values[0]);
    latx_vmovsd_store_u64(page + 8192 + 13, values[1]);
    latx_vmovsd_store_u64(page + 16384 + 120, values[2]);
    latx_vmovsd_store_u64(page + 25600, values[3]);
    latx_vmovsd_store_u64(page + 32768 + 1, values[4]);
    latx_vmovsd_store_u64(page + 40960 + 4088, values[5]);
    latx_vmovsd_store_u64(page + 49152 + 4092, values[6]);
}

static void latx_vmovsd_prepare_store_memory(uint8_t *page)
{
    latx_avx_single_fill(page, LATX_VMOVSD_PAGES * LATX_PAGE_SIZE,
                         UINT64_C(0x53544f52455344a5));
}

static int latx_vmovsd_write_store_snapshots(uint8_t *page)
{
    const uint8_t *snapshots[] = {
        page,
        page + 8192 + 13,
        page + 16384 + 120,
        page + 25600,
        latx_avx_vmovsd_rip_store,
        page + 32768 + 1,
        page + 40960 + 4064,
        page + 49152 + 4092,
    };

    for (size_t i = 0; i < sizeof(snapshots) / sizeof(snapshots[0]); ++i) {
        if (latx_avx_single_write_all(snapshots[i], 32) != 0) {
            return -1;
        }
    }
    return 0;
}

static void latx_vmovsd_fault_handler(int signal_number, void *siginfo,
                                      void *ucontext)
{
    const uint8_t *siginfo_bytes = siginfo;
    const uint8_t *ucontext_bytes = ucontext;
    uintptr_t fault_address = *(const uintptr_t *)(siginfo_bytes + 16);
    uintptr_t fpstate_address = *(const uintptr_t *)(ucontext_bytes + 224);
    struct latx_vmovsd_fault_record record = {0};

    latx_avx_single_vmovsd_fault_observe();
    record.signal_number = signal_number;
    record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
    record.fault_offset = fault_address - vmovsd_fault_base;
    if (fpstate_address != 0) {
        const uint8_t *fpstate = (const uint8_t *)fpstate_address;

        latx_vmovsd_copy(record.xmm15, fpstate + 160 + 15 * 16, 16);
        latx_vmovsd_copy(record.ymmh15, fpstate + 576 + 15 * 16, 16);
    }
    latx_vmovsd_copy(record.memory,
                     (const uint8_t *)(uintptr_t)vmovsd_fault_snapshot,
                     sizeof(record.memory));
    latx_avx_single_write_all(&record, sizeof(record));
    latx_vmovsd_exit(128 + signal_number);
}

static int latx_vmovsd_install_handler(int signal_number)
{
    struct latx_kernel_sigaction action = {
        .handler = latx_vmovsd_fault_handler,
        .flags = LATX_SA_SIGINFO | LATX_SA_RESTORER,
        .restorer = latx_avx_single_rt_sigreturn,
        .mask = 0,
    };

    return latx_vmovsd_syscall4(
        LATX_SYS_RT_SIGACTION, signal_number,
        (long)(uintptr_t)&action, 0, 8) < 0;
}

static int latx_vmovsd_fault_case(const char *name, uint8_t *load_page,
                                  uint8_t *store_page)
{
    uint8_t *page;
    uint8_t *address;
    void (*operation)(uint8_t *, int);
    int restore = 1;
    int is_store = 0;

    if (latx_vmovsd_streq(name, "load-cross-1") ||
        latx_vmovsd_streq(name, "trace-load-cross-1")) {
        page = load_page;
        address = page + 4089;
        operation = latx_avx_single_vmovsd_fault_load;
    } else if (latx_vmovsd_streq(name, "load-cross-7") ||
               latx_vmovsd_streq(name, "trace-load-cross-7")) {
        page = load_page;
        address = page + 4095;
        operation = latx_avx_single_vmovsd_fault_load;
    } else if (latx_vmovsd_streq(name, "store-cross-1") ||
               latx_vmovsd_streq(name, "trace-store-cross-1")) {
        page = store_page;
        address = page + 4089;
        operation = latx_avx_single_vmovsd_fault_store;
        is_store = 1;
    } else if (latx_vmovsd_streq(name, "store-cross-7") ||
               latx_vmovsd_streq(name, "trace-store-cross-7")) {
        page = store_page;
        address = page + 4095;
        operation = latx_avx_single_vmovsd_fault_store;
        is_store = 1;
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
    vmovsd_fault_base = (uintptr_t)page;
    vmovsd_fault_snapshot = (uintptr_t)(page + 4080);
    operation(address, restore);
    return is_store ? 91 : 90;
}

static int latx_vmovsd_mode(const char *name)
{
    if (latx_vmovsd_streq(name, "trace0-store") ||
        latx_vmovsd_streq(name, "memory0")) {
        return 1;
    }
    if (latx_vmovsd_streq(name, "trace0-load")) {
        return 2;
    }
    if (latx_vmovsd_streq(name, "trace0-register")) {
        return 3;
    }
    if (latx_vmovsd_streq(name, "trace15-store") ||
        latx_vmovsd_streq(name, "memory15")) {
        return 4;
    }
    if (latx_vmovsd_streq(name, "trace15-load")) {
        return 5;
    }
    if (latx_vmovsd_streq(name, "trace15-register")) {
        return 6;
    }
    return -1;
}

int latx_avx_single_main(long argc, char **argv)
{
    uint8_t *load_page = latx_vmovsd_map();
    uint8_t *store_page = latx_vmovsd_map();
    int mode;

    if (load_page == 0 || store_page == 0) {
        return 70;
    }
    latx_vmovsd_prepare_load_memory(load_page);
    latx_vmovsd_prepare_store_memory(store_page);

    if (argc == 1 ||
        (argc == 2 && latx_vmovsd_streq(argv[1], "reference"))) {
        if (latx_avx_single_syscall3(
                LATX_SYS_MPROTECT,
                (long)(uintptr_t)(load_page + 11 * LATX_PAGE_SIZE),
                LATX_PAGE_SIZE, LATX_PROT_NONE) < 0 ||
            latx_avx_single_syscall3(
                LATX_SYS_MPROTECT,
                (long)(uintptr_t)(store_page + 11 * LATX_PAGE_SIZE),
                LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
            return 71;
        }
        latx_avx_single_vmovsd_run(
            vmovsd_output, load_page, store_page, 0);
        return latx_avx_single_write_all(
                   vmovsd_output, sizeof(vmovsd_output)) != 0;
    }
    if (argc != 2) {
        return 72;
    }
    mode = latx_vmovsd_mode(argv[1]);
    if (mode >= 0) {
        if (latx_avx_single_syscall3(
                LATX_SYS_MPROTECT,
                (long)(uintptr_t)(load_page + 11 * LATX_PAGE_SIZE),
                LATX_PAGE_SIZE, LATX_PROT_NONE) < 0 ||
            latx_avx_single_syscall3(
                LATX_SYS_MPROTECT,
                (long)(uintptr_t)(store_page + 11 * LATX_PAGE_SIZE),
                LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
            return 71;
        }
        latx_avx_single_vmovsd_run(0, load_page, store_page, mode);
        if (latx_vmovsd_streq(argv[1], "memory0") ||
            latx_vmovsd_streq(argv[1], "memory15")) {
            return latx_vmovsd_write_store_snapshots(store_page) != 0;
        }
        return 0;
    }
    if (latx_vmovsd_install_handler(LATX_SIGSEGV) != 0 ||
        latx_vmovsd_install_handler(LATX_SIGBUS) != 0) {
        return 73;
    }
    return latx_vmovsd_fault_case(argv[1], load_page, store_page);
}
