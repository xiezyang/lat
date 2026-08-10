#!/usr/bin/env python3
"""Generate independent xzy86 fixtures for VPMASKMOVD and VPMASKMOVQ."""

from pathlib import Path
import argparse
import json


ASM = r'''/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .equ SRC, 0
    .equ MASK_ZERO, 128
    .equ MASK_ONE, 160
    .equ MASK_MIX, 192
    .equ MASK_FAULT_MIX, 224
    .equ DATA, 256
    .equ STORE_BASE, 1024

    .macro SAVE_XMM memory_offset
    vmovdqu xmmword ptr [rdi], xmm0
    vpxor xmm15, xmm15, xmm15
    vmovdqu xmmword ptr [rdi + 16], xmm15
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm14
    add rdi, 64
    .endm

    .macro SAVE_YMM memory_offset
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm14
    add rdi, 64
    .endm

    .globl latx_avx_single_@MNEMONIC@_run
latx_avx_single_@MNEMONIC@_run:
    push r12
    xor r8d, r8d

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_ZERO]
    @MNEMONIC@ xmm0, xmm1, @XMEM@ ptr [rsi + SRC]
    SAVE_XMM SRC

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_ONE]
    @MNEMONIC@ xmm0, xmm1, @XMEM@ ptr [rsi + SRC]
    SAVE_XMM SRC

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_MIX]
    @MNEMONIC@ xmm0, xmm1, @XMEM@ ptr [rsi + SRC + 3]
    SAVE_XMM SRC + 3

    vmovdqu xmm0, xmmword ptr [rsi + MASK_MIX]
    @MNEMONIC@ xmm0, xmm0, @XMEM@ ptr [rsi + SRC]
    SAVE_XMM SRC

    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rsi + MASK_MIX]
    @MNEMONIC@ ymm0, ymm1, @YMEM@ ptr [rsi + SRC]
    SAVE_YMM SRC

    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rsi + MASK_ONE]
    @MNEMONIC@ ymm0, ymm1, @YMEM@ ptr [rsi + r8*@SCALE@ + SRC + 5]
    SAVE_YMM SRC + 5

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_ZERO]
    @MNEMONIC@ @XMEM@ ptr [rsi + STORE_BASE], xmm1, xmm0
    SAVE_XMM STORE_BASE

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_ONE]
    @MNEMONIC@ @XMEM@ ptr [rsi + STORE_BASE + 64], xmm1, xmm0
    SAVE_XMM STORE_BASE + 64

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_MIX]
    @MNEMONIC@ @XMEM@ ptr [rsi + STORE_BASE + 128 + 3], xmm1, xmm0
    SAVE_XMM STORE_BASE + 128 + 3

    vmovdqu xmm0, xmmword ptr [rsi + MASK_MIX]
    @MNEMONIC@ @XMEM@ ptr [rsi + STORE_BASE + 192], xmm0, xmm0
    SAVE_XMM STORE_BASE + 192

    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rsi + MASK_MIX]
    @MNEMONIC@ @YMEM@ ptr [rsi + r8*@SCALE@ + STORE_BASE + 256 + 5], ymm1, ymm0
    SAVE_YMM STORE_BASE + 256 + 5

    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rsi + MASK_ONE]
    @MNEMONIC@ @YMEM@ ptr [rsi + r8*@SCALE@ + STORE_BASE + 320], ymm1, ymm0
    SAVE_YMM STORE_BASE + 320

    pop r12
    vzeroupper
    ret

    .globl latx_avx_single_@MNEMONIC@_fault_load
latx_avx_single_@MNEMONIC@_fault_load:
    vmovdqu xmm1, xmmword ptr [rdx]
    @MNEMONIC@ xmm0, xmm1, @XMEM@ ptr [rdi]
    ret

    .globl latx_avx_single_@MNEMONIC@_fault_load_ymm
latx_avx_single_@MNEMONIC@_fault_load_ymm:
    vmovdqu ymm1, ymmword ptr [rdx]
    @MNEMONIC@ ymm0, ymm1, @YMEM@ ptr [rdi]
    vzeroupper
    ret

    .globl latx_avx_single_@MNEMONIC@_fault_store
latx_avx_single_@MNEMONIC@_fault_store:
    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rdx]
    @MNEMONIC@ @XMEM@ ptr [rdi], xmm1, xmm0
    ret

    .globl latx_avx_single_@MNEMONIC@_fault_store_ymm
latx_avx_single_@MNEMONIC@_fault_store_ymm:
    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rdx]
    @MNEMONIC@ @YMEM@ ptr [rdi], ymm1, ymm0
    vzeroupper
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .note.GNU-stack,"",@progbits
'''


C = r'''/* SPDX-License-Identifier: GPL-2.0-only */

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

extern void latx_avx_single_@MNEMONIC@_run(uint8_t *, uint8_t *);
extern void latx_avx_single_@MNEMONIC@_fault_load(uint8_t *, uint8_t *, const uint8_t *);
extern void latx_avx_single_@MNEMONIC@_fault_load_ymm(uint8_t *, uint8_t *, const uint8_t *);
extern void latx_avx_single_@MNEMONIC@_fault_store(uint8_t *, uint8_t *, const uint8_t *);
extern void latx_avx_single_@MNEMONIC@_fault_store_ymm(uint8_t *, uint8_t *, const uint8_t *);
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
    for (unsigned i = 0; i < @ELEMENT_BYTES@; ++i)
        p[i] = (uint8_t)(value >> (8 * i));
}

static void prepare(uint8_t *p)
{
    static const uint64_t source[] = { @SOURCE_VALUES@ };
    static const uint64_t data[] = { @DATA_VALUES@ };
    static const uint64_t mask_zero[] = { @MASK_ZERO_VALUES@ };
    static const uint64_t mask_one[] = { @MASK_ONE_VALUES@ };
    static const uint64_t mask_mix[] = { @MASK_MIX_VALUES@ };
    static const uint64_t mask_fault_mix[] = { @MASK_FAULT_MIX_VALUES@ };

    latx_avx_single_fill(p, PAGES * PAGE, UINT64_C(0x564d41534b3138));
    for (unsigned i = 0; i < @ELEMENTS@; ++i) {
        set_element(p + SRC + i * @ELEMENT_BYTES@, source[i]);
        set_element(p + DATA + i * @ELEMENT_BYTES@, data[i]);
        set_element(p + MASK_ZERO + i * @ELEMENT_BYTES@, mask_zero[i]);
        set_element(p + MASK_ONE + i * @ELEMENT_BYTES@, mask_one[i]);
        set_element(p + MASK_MIX + i * @ELEMENT_BYTES@, mask_mix[i]);
        set_element(p + MASK_FAULT_MIX + i * @ELEMENT_BYTES@,
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
            latx_avx_single_@MNEMONIC@_fault_store_ymm(target, base,
                base + MASK_ZERO);
        else
            latx_avx_single_@MNEMONIC@_fault_store(target, base,
                base + MASK_ZERO);
    } else if (ymm) {
        latx_avx_single_@MNEMONIC@_fault_load_ymm(target, base,
            base + MASK_ZERO);
    } else {
        latx_avx_single_@MNEMONIC@_fault_load(target, base,
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
            latx_avx_single_@MNEMONIC@_fault_store_ymm(target, base, mask);
        else
            latx_avx_single_@MNEMONIC@_fault_store(target, base, mask);
    } else if (ymm) {
        latx_avx_single_@MNEMONIC@_fault_load_ymm(target, base, mask);
    } else {
        latx_avx_single_@MNEMONIC@_fault_load(target, base, mask);
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
        latx_avx_single_@MNEMONIC@_run(output, page);
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
'''


def c_array(values: list[int]) -> str:
    return ", ".join(f"UINT64_C(0x{value:016x})" for value in values)


def render(mnemonic: str, element_bytes: int) -> tuple[str, str]:
    elements = 16 // element_bytes
    if element_bytes == 4:
        source = [0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00,
                  0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210]
        data = [0x0F1E2D3C, 0x4B5A6978, 0x88776655, 0x44332211,
                0x10203040, 0x50607080, 0x90807060, 0x50403020]
    else:
        source = [0x1122334455667788, 0x99AABBCCDDEEFF00,
                  0x0123456789ABCDEF, 0xFEDCBA9876543210]
        data = [0x0F1E2D3C4B5A6978, 0x8877665544332211,
                0x1020304050607080, 0x9080706050403020]
    sign = 1 << (element_bytes * 8 - 1)
    source = source[:elements * 2]
    data = data[:elements * 2]
    zero = [0] * (elements * 2)
    one = [sign] * (elements * 2)
    mixed = [sign if i % 2 == 0 else 0 for i in range(elements * 2)]
    fault_mix = mixed[:]
    if element_bytes == 8:
        fault_mix[1] = sign
    replacements = {
        "MNEMONIC": mnemonic,
        "XMEM": "xmmword",
        "YMEM": "ymmword",
        "SCALE": str(element_bytes),
        "ELEMENT_BYTES": str(element_bytes),
        "ELEMENTS": str(elements * 2),
        "SOURCE_VALUES": c_array(source),
        "DATA_VALUES": c_array(data),
        "MASK_ZERO_VALUES": c_array(zero),
        "MASK_ONE_VALUES": c_array(one),
        "MASK_MIX_VALUES": c_array(mixed),
        "MASK_FAULT_MIX_VALUES": c_array(fault_mix),
    }
    asm = ASM
    c = C
    for key, value in replacements.items():
        asm = asm.replace(f"@{key}@", value)
        c = c.replace(f"@{key}@", value)
    return asm, c


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for mnemonic, width in (("vpmaskmovd", 4), ("vpmaskmovq", 8)):
        asm, c = render(mnemonic, width)
        (args.output_dir / f"latx-avx-single-{mnemonic}.S").write_text(asm)
        (args.output_dir / f"latx-avx-single-{mnemonic}.c").write_text(c)
    manifest = {
        "schema_version": 1,
        "mnemonics": ["vpmaskmovd", "vpmaskmovq"],
        "normal": {"status": 0, "stdout_bytes": 768, "records": 12},
        "mask_zero_cross_page": {
            "cases": [
                "load-xmm-zero", "load-ymm-zero",
                "store-xmm-zero", "store-ymm-zero",
            ],
            "status": 0,
            "stdout_bytes": 16,
        },
        "mask_on_cross_page": {
            "cases": [
                "load-xmm-one", "load-ymm-mix",
                "store-xmm-one", "store-ymm-mix",
            ],
            "status": 139,
            "signal": "SIGSEGV",
            "stdout_bytes": 16,
        },
        "addressing": {
            "base_displacement": True,
            "base_index_scale": [4, 8],
            "guest_index_operand": False,
        },
    }
    (args.output_dir / "wi1918-fixture-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n"
    )
    print("mnemonics=vpmaskmovd vpmaskmovq")
    print(f"output_dir={args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
