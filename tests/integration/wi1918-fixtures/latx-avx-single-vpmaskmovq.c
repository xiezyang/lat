/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9, SYS_MPROTECT = 10, SYS_RT_SIGACTION = 13, SYS_EXIT = 60,
    PROT_NONE = 0, PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20, SA_SIGINFO = 4, SA_RESTORER = 0x04000000,
    SIGBUS = 7, SIGSEGV = 11, PAGE = 4096, PAGES = 3,
    CASES = 12, RECORD_SIZE = 64, OUTPUT_SIZE = CASES * RECORD_SIZE,
    SRC = 0, MASK_ZERO = 128, MASK_ONE = 160, MASK_MIX = 192,
    MASK_FAULT_MIX = 224, DATA = 256, STORE_BASE = 1024,
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

extern void latx_avx_single_vpmaskmovq_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vpmaskmovq_fault_load(uint8_t *, uint8_t *, const uint8_t *);
extern void latx_avx_single_vpmaskmovq_fault_load_ymm(uint8_t *, uint8_t *, const uint8_t *);
extern void latx_avx_single_vpmaskmovq_fault_store(uint8_t *, uint8_t *, const uint8_t *);
extern void latx_avx_single_vpmaskmovq_fault_store_ymm(uint8_t *, uint8_t *, const uint8_t *);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t output[OUTPUT_SIZE];
static uint8_t *fault_target;
static uint8_t fault_capture[16];

static inline long syscall3(long n, long a, long b, long c)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi),
                     "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

static inline long syscall4(long n, long a, long b, long c, long d)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    register long r10 __asm__("r10") = d;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi),
                     "d"(rdx), "r"(r10) : "rcx", "r11", "memory");
    return rax;
}

static inline long syscall6(long n, long a, long b, long c, long d, long e,
                             long f)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8") = e;
    register long r9 __asm__("r9") = f;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi),
                     "d"(rdx), "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return rax;
}

static __attribute__((noreturn)) void exit_now(int status)
{
    __asm__ volatile("syscall" : : "a"((long)SYS_EXIT), "D"((long)status)
                     : "rcx", "r11", "memory");
    __builtin_unreachable();
}

static void copy_tag(const char *tag)
{
    for (unsigned i = 0; i < 8; ++i)
        fault_capture[i] = tag[i] ? (uint8_t)tag[i] : 0;
}

static int same(const char *left, const char *right)
{
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void emit_fault_record(void)
{
    for (unsigned i = 0; i < 8; ++i)
        fault_capture[8 + i] = fault_target[i];
    latx_avx_single_write_all(fault_capture, sizeof(fault_capture));
}

static void handler(int sig, void *info, void *ctx)
{
    (void)info;
    (void)ctx;
    emit_fault_record();
    exit_now(128 + sig);
}

static int install_fault_handlers(void)
{
    struct kernel_sigaction action = {
        handler, SA_SIGINFO | SA_RESTORER, latx_avx_single_rt_sigreturn, 0,
    };
    return syscall4(SYS_RT_SIGACTION, SIGSEGV, (long)(uintptr_t)&action, 0,
                    8) < 0 ||
           syscall4(SYS_RT_SIGACTION, SIGBUS, (long)(uintptr_t)&action, 0,
                    8) < 0;
}

static void set_element(uint8_t *p, uint64_t value)
{
    for (unsigned i = 0; i < 8; ++i)
        p[i] = (uint8_t)(value >> (8 * i));
}

static void prepare(uint8_t *p)
{
    static const uint64_t source[] = { UINT64_C(0x1122334455667788), UINT64_C(0x99aabbccddeeff00), UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210) };
    static const uint64_t data[] = { UINT64_C(0x0f1e2d3c4b5a6978), UINT64_C(0x8877665544332211), UINT64_C(0x1020304050607080), UINT64_C(0x9080706050403020) };
    static const uint64_t mask_zero[] = { UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000) };
    static const uint64_t mask_one[] = { UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000000) };
    static const uint64_t mask_mix[] = { UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000000) };
    static const uint64_t mask_fault_mix[] = { UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000000) };

    latx_avx_single_fill(p, PAGES * PAGE, UINT64_C(0x564d41534b3138));
    for (unsigned i = 0; i < 4; ++i) {
        set_element(p + SRC + i * 8, source[i]);
        set_element(p + DATA + i * 8, data[i]);
        set_element(p + MASK_ZERO + i * 8, mask_zero[i]);
        set_element(p + MASK_ONE + i * 8, mask_one[i]);
        set_element(p + MASK_MIX + i * 8, mask_mix[i]);
        set_element(p + MASK_FAULT_MIX + i * 8,
                    mask_fault_mix[i]);
    }
    for (unsigned i = 0; i < 6; ++i)
        latx_avx_single_fill(p + STORE_BASE + i * 64, 64,
                             UINT64_C(0x53544f52453138) + i);
}

static int run_zero(uint8_t *target, uint8_t *base, int store, int ymm)
{
    copy_tag(store ? (ymm ? "sz-ymm" : "sz-xmm")
                  : (ymm ? "lz-ymm" : "lz-xmm"));
    fault_target = target;
    if (store) {
        if (ymm)
            latx_avx_single_vpmaskmovq_fault_store_ymm(target, base,
                base + MASK_ZERO);
        else
            latx_avx_single_vpmaskmovq_fault_store(target, base,
                base + MASK_ZERO);
    } else if (ymm) {
        latx_avx_single_vpmaskmovq_fault_load_ymm(target, base,
            base + MASK_ZERO);
    } else {
        latx_avx_single_vpmaskmovq_fault_load(target, base,
            base + MASK_ZERO);
    }
    emit_fault_record();
    return 0;
}

static int run_fault(uint8_t *target, uint8_t *base, int store, int ymm,
                     int mixed)
{
    const uint8_t *mask = base + (mixed ? MASK_FAULT_MIX : MASK_ONE);
    copy_tag(store ? (ymm ? "sf-ymm" : "sf-xmm")
                  : (ymm ? "lf-ymm" : "lf-xmm"));
    fault_target = target;
    if (store) {
        if (ymm)
            latx_avx_single_vpmaskmovq_fault_store_ymm(target, base, mask);
        else
            latx_avx_single_vpmaskmovq_fault_store(target, base, mask);
    } else if (ymm) {
        latx_avx_single_vpmaskmovq_fault_load_ymm(target, base, mask);
    } else {
        latx_avx_single_vpmaskmovq_fault_load(target, base, mask);
    }
    return 91;
}

int latx_avx_single_main(long argc, char **argv)
{
    long map = syscall6(SYS_MMAP, 0, PAGES * PAGE, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint8_t *page;

    if (map < 0)
        return 70;
    page = (uint8_t *)(uintptr_t)map;
    prepare(page);
    if (argc == 1) {
        latx_avx_single_vpmaskmovq_run(output, page);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc != 2 || install_fault_handlers())
        return 72;
    if (syscall3(SYS_MPROTECT, (long)(uintptr_t)(page + PAGE), PAGE,
                 PROT_NONE) < 0)
        return 71;
    uint8_t *target = page + PAGE - 8;
    if (same(argv[1], "load-xmm-zero"))
        return run_zero(target, page, 0, 0);
    if (same(argv[1], "load-ymm-zero"))
        return run_zero(target, page, 0, 1);
    if (same(argv[1], "store-xmm-zero"))
        return run_zero(target, page, 1, 0);
    if (same(argv[1], "store-ymm-zero"))
        return run_zero(target, page, 1, 1);
    if (same(argv[1], "load-xmm-one"))
        return run_fault(target, page, 0, 0, 0);
    if (same(argv[1], "load-ymm-mix"))
        return run_fault(target, page, 0, 1, 1);
    if (same(argv[1], "store-xmm-one"))
        return run_fault(target, page, 1, 0, 0);
    if (same(argv[1], "store-ymm-mix"))
        return run_fault(target, page, 1, 1, 1);
    return 74;
}
