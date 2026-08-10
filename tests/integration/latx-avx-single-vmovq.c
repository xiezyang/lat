/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9,
    SYS_MPROTECT = 10,
    SYS_MUNMAP = 11,
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
    RESULT_BYTES = 14 * 16,
    STORE_MEMORY_BYTES = PAGE_SIZE + 32,
};

struct vmovq_header {
    uint32_t magic;
    uint32_t pattern;
    uint64_t value;
};

struct vmovq_fault_result {
    uint32_t kind;
    uint32_t signal;
    uint8_t memory[16];
};

extern uint64_t latx_avx_vmovq_rip_input;
extern uint8_t latx_avx_vmovq_rip_store[16];

void latx_avx_single_vmovq(uint64_t value, const uint8_t *load_memory,
                           const uint8_t *load_page_tail,
                           uint8_t *register_results,
                           uint8_t *store_memory,
                           uint8_t *store_page_tail);
void latx_avx_single_vmovq_fault_load(const uint8_t *address);
void latx_avx_single_vmovq_fault_store(uint8_t *address, uint64_t value);

static uint8_t load_memory[STORE_MEMORY_BYTES];
static uint8_t store_memory[STORE_MEMORY_BYTES];
static uint8_t register_results[RESULT_BYTES];

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

static void store_u64(uint8_t *address, uint64_t value)
{
    for (unsigned int i = 0; i < 8; ++i) {
        address[i] = (uint8_t)(value >> (i * 8));
    }
}

static int write_piece(const void *data, size_t size)
{
    return latx_avx_single_write_all(data, size);
}

static int write_normal_result(unsigned int pattern, uint64_t value,
                               uint8_t *load_pages,
                               uint8_t *store_pages)
{
    struct vmovq_header header = {
        .magic = UINT32_C(0x514f4d56),
        .pattern = pattern,
        .value = value,
    };
    const size_t snapshots[][2] = {
        {0, 16},
        {56, 16},
        {112, 16},
        {4096, 16},
        {249, 16},
    };

    if (write_piece(&header, sizeof(header)) != 0 ||
        write_piece(register_results, sizeof(register_results)) != 0) {
        return -1;
    }
    for (size_t i = 0; i < sizeof(snapshots) / sizeof(snapshots[0]); ++i) {
        if (write_piece(store_memory + snapshots[i][0], snapshots[i][1]) != 0) {
            return -1;
        }
    }
    if (write_piece(latx_avx_vmovq_rip_store,
                    sizeof(latx_avx_vmovq_rip_store)) != 0 ||
        write_piece(store_pages + PAGE_SIZE - 16, 16) != 0) {
        return -1;
    }

    (void)load_pages;
    return 0;
}

static int run_one_pattern(unsigned int pattern, uint64_t value,
                           uint8_t *load_pages, uint8_t *store_pages)
{
    fill_bytes(load_memory, sizeof(load_memory), 0xa5);
    fill_bytes(store_memory, sizeof(store_memory), 0x5a);
    fill_bytes(register_results, sizeof(register_results), 0xcc);
    fill_bytes(latx_avx_vmovq_rip_store,
               sizeof(latx_avx_vmovq_rip_store), 0x5a);
    fill_bytes(load_pages + PAGE_SIZE - 16, 16, 0xa5);
    fill_bytes(store_pages + PAGE_SIZE - 16, 16, 0x5a);

    store_u64(load_memory + 0, value);
    store_u64(load_memory + 24, value);
    store_u64(load_memory + 120, value);
    store_u64(load_memory + 4096, value);
    store_u64(load_memory + 257, value);
    store_u64(load_pages + PAGE_SIZE - 8, value);
    latx_avx_vmovq_rip_input = value;

    latx_avx_single_vmovq(value, load_memory, load_pages + PAGE_SIZE - 8,
                          register_results, store_memory,
                          store_pages + PAGE_SIZE - 8);
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

static unsigned int termination_signal(int status)
{
    return (unsigned int)status & 0x7f;
}

static int run_fault_tests(uint8_t *load_pages, uint8_t *store_pages)
{
    struct vmovq_fault_result result;
    long child;
    int status;

    fill_bytes(load_pages + PAGE_SIZE - 16, 16, 0x6c);
    child = latx_avx_single_syscall3(SYS_FORK, 0, 0, 0);
    if (child == 0) {
        latx_avx_single_vmovq_fault_load(load_pages + PAGE_SIZE - 4);
        latx_avx_single_exit(90);
    }
    if (child < 0 || (status = wait_for_fault(child)) < 0) {
        return -1;
    }
    result.kind = 1;
    result.signal = termination_signal(status);
    if (result.signal != 11) {
        return -1;
    }
    for (unsigned int i = 0; i < sizeof(result.memory); ++i) {
        result.memory[i] = load_pages[PAGE_SIZE - 16 + i];
    }
    if (write_piece(&result, sizeof(result)) != 0) {
        return -1;
    }

    fill_bytes(store_pages + PAGE_SIZE - 16, 16, 0x6c);
    child = latx_avx_single_syscall3(SYS_FORK, 0, 0, 0);
    if (child == 0) {
        latx_avx_single_vmovq_fault_store(
            store_pages + PAGE_SIZE - 4, UINT64_C(0x8877665544332211));
        latx_avx_single_exit(91);
    }
    if (child < 0 || (status = wait_for_fault(child)) < 0) {
        return -1;
    }
    result.kind = 2;
    result.signal = termination_signal(status);
    if (result.signal != 11) {
        return -1;
    }
    for (unsigned int i = 0; i < sizeof(result.memory); ++i) {
        result.memory[i] = store_pages[PAGE_SIZE - 16 + i];
    }
    return write_piece(&result, sizeof(result));
}

int latx_avx_single_main(long argc, char **argv)
{
    uint8_t random_bytes[8];
    uint8_t *load_pages;
    uint8_t *store_pages;
    uint64_t patterns[6] = {
        UINT64_C(0),
        UINT64_MAX,
        UINT64_C(0x8000000000000000),
        UINT64_C(0x0706050403020100),
        UINT64_C(0xaa55aa55aa55aa55),
        0,
    };

    (void)argc;
    (void)argv;
    latx_avx_single_fill(random_bytes, sizeof(random_bytes),
                         UINT64_C(0x1623c0decafef00d));
    for (unsigned int i = 0; i < sizeof(random_bytes); ++i) {
        patterns[5] |= (uint64_t)random_bytes[i] << (i * 8);
    }

    load_pages = map_guarded_pages();
    store_pages = map_guarded_pages();
    if (!load_pages || !store_pages) {
        return 2;
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
