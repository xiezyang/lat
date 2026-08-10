/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9,
    SYS_MPROTECT = 10,
    SYS_MUNMAP = 11,
    SYS_RT_SIGACTION = 13,
    SYS_FORK = 57,
    SYS_WAIT4 = 61,
    SYS_EXIT = 60,
    PAGE_SIZE = 4096,
    MAPPING_SIZE = PAGE_SIZE * 2,
    PROT_NONE = 0,
    PROT_READ = 1,
    PROT_WRITE = 2,
    MAP_SHARED = 1,
    MAP_ANONYMOUS = 0x20,
    SA_SIGINFO = 4,
    SA_RESTORER = 0x04000000,
    SIGSEGV = 11,
    REGISTER_RESULT_BYTES = 32 * 16,
    MEMORY_RESULT_BYTES = 10 * 16,
    STORE_MEMORY_BYTES = PAGE_SIZE + 32,
};

struct vmovd_header {
    uint32_t magic;
    uint32_t pattern;
    uint32_t value;
    uint32_t reserved;
};

struct vmovd_fault_result {
    uint32_t kind;
    uint32_t signal;
    uint8_t xmm0[16];
    uint8_t ymmh0[16];
    uint8_t memory[16];
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

extern uint32_t latx_avx_vmovd_rip_input;
extern uint8_t latx_avx_vmovd_rip_store[16];
extern uint8_t latx_avx_vmovd_xmm_seed[16];

void latx_avx_single_vmovd_registers(uint32_t value);
void latx_avx_single_vmovd_memory(uint32_t value,
                                  const uint8_t *load_memory,
                                  const uint8_t *load_page_tail,
                                  uint8_t *memory_results,
                                  uint8_t *store_memory,
                                  uint8_t *store_page_tail);
void latx_avx_single_vmovd_fault_load(const uint8_t *address);
void latx_avx_single_vmovd_fault_store(uint8_t *address, uint32_t value);
void latx_avx_single_vmovd_rt_sigreturn(void);

static uint8_t load_memory[STORE_MEMORY_BYTES];
static uint8_t store_memory[STORE_MEMORY_BYTES];
uint8_t latx_avx_vmovd_register_results[REGISTER_RESULT_BYTES];
static uint8_t memory_results[MEMORY_RESULT_BYTES];
static volatile uint32_t vmovd_fault_kind;
static volatile uintptr_t vmovd_fault_snapshot;

static inline long latx_avx_single_syscall6(long number, long arg0,
                                            long arg1, long arg2,
                                            long arg3, long arg4,
                                            long arg5)
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

static void latx_avx_single_exit(int status)
{
    latx_avx_single_syscall3(SYS_EXIT, status, 0, 0);
    for (;;) {
    }
}

static uint8_t *map_guarded_pages(void)
{
    long result = latx_avx_single_syscall6(
        SYS_MMAP, 0, MAPPING_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    uint8_t *mapping = (uint8_t *)(uintptr_t)result;

    if ((unsigned long)result >= (unsigned long)-4095) {
        return NULL;
    }
    if (latx_avx_single_syscall3(SYS_MPROTECT,
                                 (long)(uintptr_t)(mapping + PAGE_SIZE),
                                 PAGE_SIZE, PROT_NONE) != 0) {
        latx_avx_single_syscall3(SYS_MUNMAP, (long)(uintptr_t)mapping,
                                 MAPPING_SIZE, 0);
        return NULL;
    }
    return mapping;
}

static void fill_bytes(uint8_t *buffer, size_t size, uint8_t value)
{
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = value;
    }
}

static void store_u32(uint8_t *address, uint32_t value)
{
    for (unsigned int i = 0; i < 4; ++i) {
        address[i] = (uint8_t)(value >> (i * 8));
    }
}

static int write_piece(const void *data, size_t size)
{
    return latx_avx_single_write_all(data, size);
}

static void copy_bytes(uint8_t *dest, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dest[i] = src[i];
    }
}

static int bytes_are(const uint8_t *bytes, size_t size, uint8_t value)
{
    for (size_t i = 0; i < size; ++i) {
        if (bytes[i] != value) {
            return 0;
        }
    }
    return 1;
}

static int strings_equal(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void vmovd_fault_handler(int signal, void *siginfo, void *ucontext)
{
    uint8_t *ucontext_bytes = ucontext;
    uintptr_t fpstate_address = *(uintptr_t *)(ucontext_bytes + 224);
    struct vmovd_fault_result result = {
        .kind = vmovd_fault_kind,
        .signal = (uint32_t)signal,
    };

    (void)siginfo;
    if (fpstate_address != 0) {
        const uint8_t *fpstate = (const uint8_t *)fpstate_address;

        copy_bytes(result.xmm0, fpstate + 160, sizeof(result.xmm0));
        copy_bytes(result.ymmh0, fpstate + 576, sizeof(result.ymmh0));
    }
    copy_bytes(result.memory,
               (const uint8_t *)(uintptr_t)vmovd_fault_snapshot,
               sizeof(result.memory));
    if (write_piece(&result, sizeof(result)) != 0) {
        latx_avx_single_exit(97);
    }
    if (result.kind >= 11 && result.kind <= 13 &&
        !bytes_are(result.xmm0, sizeof(result.xmm0), 0xa7)) {
        latx_avx_single_exit(96);
    }
    latx_avx_single_exit(128 + signal);
}

static int install_fault_handler(int signal)
{
    struct kernel_sigaction action = {
        .handler = vmovd_fault_handler,
        .flags = SA_SIGINFO | SA_RESTORER,
        .restorer = latx_avx_single_vmovd_rt_sigreturn,
        .mask = 0,
    };

    return latx_avx_single_syscall6(
        SYS_RT_SIGACTION, signal, (long)(uintptr_t)&action, 0, 8, 0, 0) < 0;
}

static int write_normal_result(unsigned int pattern, uint32_t value,
                               uint8_t *load_pages,
                               uint8_t *store_pages)
{
    struct vmovd_header header = {
        .magic = UINT32_C(0x514f4d56),
        .pattern = pattern,
        .value = value,
        .reserved = 0,
    };
    const size_t snapshots[][2] = {
        {0, 16},
        {56, 16},
        {112, 16},
        {4096, 16},
        {249, 16},
    };

    if (write_piece(&header, sizeof(header)) != 0 ||
        write_piece(latx_avx_vmovd_register_results,
                    sizeof(latx_avx_vmovd_register_results)) != 0 ||
        write_piece(memory_results, sizeof(memory_results)) != 0) {
        return -1;
    }
    for (size_t i = 0; i < sizeof(snapshots) / sizeof(snapshots[0]); ++i) {
        if (write_piece(store_memory + snapshots[i][0], snapshots[i][1]) != 0) {
            return -1;
        }
    }
    if (write_piece(latx_avx_vmovd_rip_store,
                    sizeof(latx_avx_vmovd_rip_store)) != 0 ||
        write_piece(store_pages + PAGE_SIZE - 16, 16) != 0) {
        return -1;
    }

    (void)load_pages;
    return 0;
}

static int run_one_pattern(unsigned int pattern, uint32_t value,
                           uint8_t *load_pages, uint8_t *store_pages)
{
    fill_bytes(load_memory, sizeof(load_memory), 0xa5);
    fill_bytes(store_memory, sizeof(store_memory), 0x5a);
    fill_bytes(latx_avx_vmovd_register_results,
               sizeof(latx_avx_vmovd_register_results), 0xcc);
    fill_bytes(memory_results, sizeof(memory_results), 0xcc);
    fill_bytes(latx_avx_vmovd_rip_store,
               sizeof(latx_avx_vmovd_rip_store), 0x5a);
    fill_bytes(load_pages + PAGE_SIZE - 16, 16, 0xa5);
    fill_bytes(store_pages + PAGE_SIZE - 16, 16, 0x5a);

    store_u32(load_memory + 0, value);
    store_u32(load_memory + 24, value);
    store_u32(load_memory + 120, value);
    store_u32(load_memory + 4096, value);
    store_u32(load_memory + 257, value);
    store_u32(load_pages + PAGE_SIZE - 4, value);
    latx_avx_vmovd_rip_input = value;
    store_u32(latx_avx_vmovd_xmm_seed, value);

    latx_avx_single_vmovd_registers(value);
    latx_avx_single_vmovd_memory(value, load_memory,
                                 load_pages + PAGE_SIZE - 4,
                                 memory_results, store_memory,
                                 store_pages + PAGE_SIZE - 4);
    for (unsigned int i = 0; i < 12; ++i) {
        if (memory_results[128 + i] != memory_results[144 + i]) {
            return -1;
        }
    }
    return write_normal_result(pattern, value, load_pages, store_pages);
}

static int wait_for_fault(long child)
{
    int status = 0;
    long waited = latx_avx_single_syscall6(
        SYS_WAIT4, child, (long)(uintptr_t)&status, 0, 0, 0, 0);

    if (waited != child) {
        return -1;
    }
    return status;
}

static int handled_fault_succeeded(int status)
{
    return ((unsigned int)status & 0x7f) == 0 &&
           (((unsigned int)status >> 8) & 0xff) == 128 + SIGSEGV;
}

static int run_fault_tests(uint8_t *load_pages, uint8_t *store_pages)
{
    long child;
    int status;

    for (unsigned int crossing = 1; crossing <= 3; ++crossing) {
        fill_bytes(load_pages + PAGE_SIZE - 16, 16, 0x6c);
        vmovd_fault_kind = 10 + crossing;
        vmovd_fault_snapshot =
            (uintptr_t)(load_pages + PAGE_SIZE - 16);
        child = latx_avx_single_syscall3(SYS_FORK, 0, 0, 0);
        if (child == 0) {
            latx_avx_single_vmovd_fault_load(
                load_pages + PAGE_SIZE - 4 + crossing);
            latx_avx_single_exit(90);
        }
        if (child < 0 || (status = wait_for_fault(child)) < 0) {
            return -1;
        }
        if (!handled_fault_succeeded(status)) {
            return -1;
        }

        fill_bytes(store_pages + PAGE_SIZE - 16, 16, 0x6c);
        vmovd_fault_kind = 20 + crossing;
        vmovd_fault_snapshot =
            (uintptr_t)(store_pages + PAGE_SIZE - 16);
        child = latx_avx_single_syscall3(SYS_FORK, 0, 0, 0);
        if (child == 0) {
            latx_avx_single_vmovd_fault_store(
                store_pages + PAGE_SIZE - 4 + crossing,
                UINT32_C(0x44332211));
            latx_avx_single_exit(91);
        }
        if (child < 0 || (status = wait_for_fault(child)) < 0) {
            return -1;
        }
        if (!handled_fault_succeeded(status)) {
            return -1;
        }
    }

    fill_bytes(store_pages + 56, 16, 0x6c);
    if (latx_avx_single_syscall3(SYS_MPROTECT,
                                 (long)(uintptr_t)store_pages,
                                 PAGE_SIZE, PROT_READ) != 0) {
        return -1;
    }
    vmovd_fault_kind = 30;
    vmovd_fault_snapshot = (uintptr_t)(store_pages + 56);
    child = latx_avx_single_syscall3(SYS_FORK, 0, 0, 0);
    if (child == 0) {
        latx_avx_single_vmovd_fault_store(store_pages + 64,
                                          UINT32_C(0x88776655));
        latx_avx_single_exit(92);
    }
    if (child < 0 || (status = wait_for_fault(child)) < 0) {
        return -1;
    }
    return handled_fault_succeeded(status) ? 0 : -1;
}

static int run_one_load_fault(const char *name, uint8_t *load_pages)
{
    unsigned int crossing;

    if (strings_equal(name, "fault-load-cross-1")) {
        crossing = 1;
    } else if (strings_equal(name, "fault-load-cross-2")) {
        crossing = 2;
    } else if (strings_equal(name, "fault-load-cross-3")) {
        crossing = 3;
    } else {
        return -1;
    }
    fill_bytes(load_pages + PAGE_SIZE - 16, 16, 0x6c);
    vmovd_fault_kind = 10 + crossing;
    vmovd_fault_snapshot = (uintptr_t)(load_pages + PAGE_SIZE - 16);
    latx_avx_single_vmovd_fault_load(
        load_pages + PAGE_SIZE - 4 + crossing);
    return 90;
}

int latx_avx_single_main(long argc, char **argv)
{
    uint8_t random_bytes[4];
    uint8_t *load_pages;
    uint8_t *store_pages;
    uint32_t patterns[6] = {
        UINT32_C(0),
        UINT32_MAX,
        UINT32_C(0x80000000),
        UINT32_C(0x7fffffff),
        UINT32_C(0x03020100),
        0,
    };

    latx_avx_single_fill(random_bytes, sizeof(random_bytes),
                         UINT64_C(0x1623c0decafef00d));
    for (unsigned int i = 0; i < sizeof(random_bytes); ++i) {
        patterns[5] |= (uint32_t)random_bytes[i] << (i * 8);
    }

    load_pages = map_guarded_pages();
    store_pages = map_guarded_pages();
    if (!load_pages || !store_pages) {
        return 2;
    }
    if (install_fault_handler(SIGSEGV) != 0) {
        return 5;
    }
    if (argc == 2) {
        int status = run_one_load_fault(argv[1], load_pages);

        return status < 0 ? 6 : status;
    }
    if (argc != 1) {
        return 6;
    }
    for (unsigned int i = 0; i < sizeof(patterns) / sizeof(patterns[0]); ++i) {
        if (run_one_pattern(i, patterns[i], load_pages, store_pages) != 0) {
            return 3;
        }
    }
    if (run_fault_tests(load_pages, store_pages) != 0) {
        return 4;
    }
    latx_avx_single_syscall3(SYS_MUNMAP, (long)(uintptr_t)load_pages,
                             MAPPING_SIZE, 0);
    latx_avx_single_syscall3(SYS_MUNMAP, (long)(uintptr_t)store_pages,
                             MAPPING_SIZE, 0);
    return 0;
}
