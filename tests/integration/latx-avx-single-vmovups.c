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
    LATX_MAPPING_SIZE = 3 * LATX_PAGE_SIZE,
    LATX_VMOVUPS_LOAD_OUTPUT_SIZE = 1328,
    LATX_VMOVUPS_OUTPUT_SIZE = 4544,
    LATX_VMOVUPS_REGISTER_CASES = 12,
    LATX_VMOVUPS_REGISTER_OUTPUT_SIZE =
        LATX_VMOVUPS_REGISTER_CASES * 32,
};

struct latx_kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

struct latx_vmovups_fault_record {
    int32_t signal_number;
    int32_t signal_code;
    uint64_t fault_offset;
    uint8_t xmm15[16];
    uint8_t memory[48];
};

extern void latx_avx_single_vmovups_xmm_load(uint8_t *, uint8_t *);
extern void latx_avx_single_vmovups_ymm_load(uint8_t *, uint8_t *);
extern void latx_avx_single_vmovups_xmm_store(uint8_t *);
extern void latx_avx_single_vmovups_ymm_store(uint8_t *);
extern void latx_avx_single_vmovups_registers(uint8_t *, uint8_t *);
extern void latx_avx_single_vmovups_fault_xmm_load(uint8_t *);
extern void latx_avx_single_vmovups_fault_xmm_store(uint8_t *);
extern void latx_avx_single_vmovups_fault_ymm_load(uint8_t *);
extern void latx_avx_single_vmovups_fault_ymm_store(uint8_t *);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t vmovups_load_output[LATX_VMOVUPS_LOAD_OUTPUT_SIZE]
    __attribute__((aligned(32)));
static uint8_t vmovups_register_output[LATX_VMOVUPS_REGISTER_OUTPUT_SIZE]
    __attribute__((aligned(32)));
static volatile uintptr_t vmovups_fault_base;
static volatile uintptr_t vmovups_snapshot;

static inline long latx_vmovups_syscall4(long number, long arg0,
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

static inline long latx_vmovups_syscall6(long number, long arg0,
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

static __attribute__((noreturn)) void latx_vmovups_exit(int status)
{
    __asm__ volatile("syscall"
                     :
                     : "a"((long)LATX_SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void latx_vmovups_copy(uint8_t *dest, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dest[i] = src[i];
    }
}

static uint8_t *latx_vmovups_map(size_t size)
{
    long mapping = latx_vmovups_syscall6(
        LATX_SYS_MMAP, 0, (long)size,
        LATX_PROT_READ | LATX_PROT_WRITE,
        LATX_MAP_PRIVATE | LATX_MAP_ANONYMOUS, -1, 0);

    if ((unsigned long)mapping >= (unsigned long)-4095) {
        return 0;
    }
    return (uint8_t *)(uintptr_t)mapping;
}

static void latx_vmovups_fault_handler(int signal_number, void *siginfo,
                                       void *ucontext)
{
    const uint8_t *siginfo_bytes = siginfo;
    const uint8_t *ucontext_bytes = ucontext;
    uintptr_t address = *(const uintptr_t *)(siginfo_bytes + 16);
    uintptr_t fpstate_address = *(const uintptr_t *)(ucontext_bytes + 224);
    struct latx_vmovups_fault_record record = {0};

    record.signal_number = signal_number;
    record.signal_code = *(const int32_t *)(siginfo_bytes + 8);
    record.fault_offset = address == 0 ? 0 : address - vmovups_fault_base;
    if (fpstate_address != 0) {
        const uint8_t *fpstate = (const uint8_t *)fpstate_address;
        latx_vmovups_copy(record.xmm15, fpstate + 160 + 15 * 16, 16);
    }
    latx_vmovups_copy(record.memory,
                      (const uint8_t *)(uintptr_t)vmovups_snapshot,
                      sizeof(record.memory));
    latx_avx_single_write_all(&record, sizeof(record));
    latx_vmovups_exit(128 + signal_number);
}

static int latx_vmovups_install_handler(int signal_number)
{
    struct latx_kernel_sigaction action = {
        .handler = latx_vmovups_fault_handler,
        .flags = LATX_SA_SIGINFO | LATX_SA_RESTORER,
        .restorer = latx_avx_single_rt_sigreturn,
        .mask = 0,
    };

    return latx_vmovups_syscall4(
        LATX_SYS_RT_SIGACTION, signal_number,
        (long)(uintptr_t)&action, 0, 8) < 0;
}

static int latx_vmovups_streq(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static int latx_vmovups_write_normal(uint8_t *dest)
{
    if (latx_avx_single_write_all(vmovups_load_output,
                                  sizeof(vmovups_load_output)) != 0) {
        return -1;
    }
    for (size_t i = 0; i < 16; ++i) {
        if (latx_avx_single_write_all(dest + i * 64, 64) != 0) {
            return -1;
        }
    }
    for (size_t i = 0; i < 32; ++i) {
        if (latx_avx_single_write_all(dest + 1280 + i * 64, 64) != 0) {
            return -1;
        }
    }
    if (latx_avx_single_write_all(dest + 4064, 64) != 0 ||
        latx_avx_single_write_all(dest + 8144, 80) != 0) {
        return -1;
    }
    return 0;
}

static void latx_vmovups_run_normal(uint8_t *source, uint8_t *dest)
{
    size_t output_offset = 0;

    for (size_t i = 0; i < 16; ++i) {
        latx_avx_single_vmovups_xmm_load(
            source + i, vmovups_load_output + output_offset);
        output_offset += 16;
    }
    latx_avx_single_vmovups_xmm_load(
        source + 4088, vmovups_load_output + output_offset);
    output_offset += 16;
    for (size_t i = 0; i < 32; ++i) {
        latx_avx_single_vmovups_ymm_load(
            source + i, vmovups_load_output + output_offset);
        output_offset += 32;
    }
    latx_avx_single_vmovups_ymm_load(
        source + 4080, vmovups_load_output + output_offset);

    for (size_t i = 0; i < 16; ++i) {
        latx_avx_single_vmovups_xmm_store(dest + i * 64 + i);
    }
    for (size_t i = 0; i < 32; ++i) {
        latx_avx_single_vmovups_ymm_store(dest + 1280 + i * 64 + i);
    }
    latx_avx_single_vmovups_xmm_store(dest + 4088);
    latx_avx_single_vmovups_ymm_store(dest + 8176);
}

static int latx_vmovups_fault_case(const char *name, uint8_t *page)
{
    uint8_t *address = 0;
    void (*operation)(uint8_t *) = 0;

    if (latx_vmovups_streq(name, "xmm-load-cross-1")) {
        address = page + 4081;
        operation = latx_avx_single_vmovups_fault_xmm_load;
    } else if (latx_vmovups_streq(name, "xmm-load-cross-15")) {
        address = page + 4095;
        operation = latx_avx_single_vmovups_fault_xmm_load;
    } else if (latx_vmovups_streq(name, "xmm-store-cross-1")) {
        address = page + 4081;
        operation = latx_avx_single_vmovups_fault_xmm_store;
    } else if (latx_vmovups_streq(name, "xmm-store-cross-15")) {
        address = page + 4095;
        operation = latx_avx_single_vmovups_fault_xmm_store;
    } else if (latx_vmovups_streq(name, "ymm-load-cross-1")) {
        address = page + 4065;
        operation = latx_avx_single_vmovups_fault_ymm_load;
    } else if (latx_vmovups_streq(name, "ymm-load-cross-15")) {
        address = page + 4079;
        operation = latx_avx_single_vmovups_fault_ymm_load;
    } else if (latx_vmovups_streq(name, "ymm-load-cross-16")) {
        address = page + 4080;
        operation = latx_avx_single_vmovups_fault_ymm_load;
    } else if (latx_vmovups_streq(name, "ymm-load-cross-31")) {
        address = page + 4095;
        operation = latx_avx_single_vmovups_fault_ymm_load;
    } else if (latx_vmovups_streq(name, "ymm-store-cross-1")) {
        address = page + 4065;
        operation = latx_avx_single_vmovups_fault_ymm_store;
    } else if (latx_vmovups_streq(name, "ymm-store-cross-15")) {
        address = page + 4079;
        operation = latx_avx_single_vmovups_fault_ymm_store;
    } else if (latx_vmovups_streq(name, "ymm-store-cross-16")) {
        address = page + 4080;
        operation = latx_avx_single_vmovups_fault_ymm_store;
    } else if (latx_vmovups_streq(name, "ymm-store-cross-31")) {
        address = page + 4095;
        operation = latx_avx_single_vmovups_fault_ymm_store;
    } else {
        return 74;
    }

    vmovups_fault_base = (uintptr_t)page;
    vmovups_snapshot = (uintptr_t)(page + 4048);
    operation(address);
    return 90;
}

int latx_avx_single_main(long argc, char **argv)
{
    uint8_t *source = latx_vmovups_map(LATX_MAPPING_SIZE);
    uint8_t *dest = latx_vmovups_map(LATX_MAPPING_SIZE);

    if (source == 0 || dest == 0) {
        return 70;
    }
    latx_avx_single_fill(source, LATX_MAPPING_SIZE,
                         UINT64_C(0xd1823f6a9754b0ce));
    latx_avx_single_fill(dest, LATX_MAPPING_SIZE,
                         UINT64_C(0x41a9c67d2eb8035f));

    if (argc == 1) {
        latx_vmovups_run_normal(source, dest);
        return latx_vmovups_write_normal(dest) != 0;
    }
    if (argc != 2) {
        return 72;
    }
    if (latx_vmovups_streq(argv[1], "register-reference")) {
        latx_avx_single_vmovups_registers(vmovups_register_output, source);
        return latx_avx_single_write_all(
            vmovups_register_output,
            sizeof(vmovups_register_output)) != 0;
    }
    if (latx_vmovups_streq(argv[1], "register-trace")) {
        latx_avx_single_vmovups_registers(0, source);
        return 0;
    }
    if (latx_vmovups_install_handler(LATX_SIGSEGV) != 0 ||
        latx_vmovups_install_handler(LATX_SIGBUS) != 0) {
        return 73;
    }
    if (latx_avx_single_syscall3(
            LATX_SYS_MPROTECT,
            (long)(uintptr_t)(source + LATX_PAGE_SIZE),
            LATX_PAGE_SIZE, LATX_PROT_NONE) < 0) {
        return 71;
    }
    return latx_vmovups_fault_case(argv[1], source);
}
