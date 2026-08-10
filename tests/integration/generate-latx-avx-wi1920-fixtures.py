#!/usr/bin/env python3
"""Generate independent xzy86 fixtures for the four AVX VUNPCK mnemonics."""

from pathlib import Path
import argparse
import json


MNEMONICS = ("vunpckhpd", "vunpckhps", "vunpcklpd", "vunpcklps")


ASM = r'''/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro SAVE_RESULT
    vmovdqu ymmword ptr [rdi], ymm2
    add rdi, 32
    .endm

    .globl latx_avx_single_@MNEMONIC@_run
    .type latx_avx_single_@MNEMONIC@_run, @function
latx_avx_single_@MNEMONIC@_run:
    /* XMM register source, destination aliases src1, src2, and both. */
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    @MNEMONIC@ xmm2, xmm0, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    @MNEMONIC@ xmm0, xmm0, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    @MNEMONIC@ xmm1, xmm0, xmm1
    vmovdqu xmm2, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    @MNEMONIC@ xmm0, xmm0, xmm0
    vmovdqu xmm2, xmm0
    SAVE_RESULT

    /* XMM memory source, with destination/src1 aliases. */
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    @MNEMONIC@ xmm2, xmm0, xmmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    @MNEMONIC@ xmm0, xmm0, xmmword ptr [rip + input_b_unaligned]
    vmovdqu xmm2, xmm0
    SAVE_RESULT

    /* YMM register source, covering both independent 128-bit lanes. */
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    @MNEMONIC@ ymm2, ymm0, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    @MNEMONIC@ ymm0, ymm0, ymm1
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    @MNEMONIC@ ymm1, ymm0, ymm1
    vmovdqu ymm2, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    @MNEMONIC@ ymm0, ymm0, ymm0
    vmovdqu ymm2, ymm0
    SAVE_RESULT

    /* YMM memory source, including an unaligned legal address. */
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    @MNEMONIC@ ymm2, ymm0, ymmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    @MNEMONIC@ ymm0, ymm0, ymmword ptr [rip + input_b_unaligned]
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vzeroupper
    ret

    .globl latx_avx_single_@MNEMONIC@_fault_xmm
    .type latx_avx_single_@MNEMONIC@_fault_xmm, @function
latx_avx_single_@MNEMONIC@_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    @MNEMONIC@ xmm2, xmm0, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_@MNEMONIC@_fault_ymm
    .type latx_avx_single_@MNEMONIC@_fault_ymm, @function
latx_avx_single_@MNEMONIC@_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    @MNEMONIC@ ymm2, ymm0, ymmword ptr [rdi]
    vzeroupper
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .rodata
    .balign 32
input_a:
@INPUT_A@
input_b_unaligned:
    .byte 0x5a
@INPUT_B@
input_b:
@INPUT_B@

    .section .note.GNU-stack,"",@progbits
'''


C = r'''/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9, SYS_MPROTECT = 10, SYS_RT_SIGACTION = 13, SYS_EXIT = 60,
    PROT_NONE = 0, PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20, SA_SIGINFO = 4, SA_RESTORER = 0x04000000,
    SIGBUS = 7, SIGSEGV = 11, PAGE = 4096, PAGES = 2,
    CASES = 12, RECORD_SIZE = 32, OUTPUT_SIZE = CASES * RECORD_SIZE,
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

extern void latx_avx_single_@MNEMONIC@_run(uint8_t *, uint8_t *);
extern void latx_avx_single_@MNEMONIC@_fault_xmm(uint8_t *);
extern void latx_avx_single_@MNEMONIC@_fault_ymm(uint8_t *);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t output[OUTPUT_SIZE];
static uint8_t fault_capture[16];
static uint8_t *fault_target;
static const char *fault_name;

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

static inline long syscall6(long n, long a, long b, long c, long d,
                             long e, long f)
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

static int same(const char *left, const char *right)
{
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void copy_tag(void)
{
    for (unsigned i = 0; i < 8; ++i)
        fault_capture[i] = fault_name[i] ? (uint8_t)fault_name[i] : 0;
}

static void fault_handler(int sig, void *info, void *ctx)
{
    (void)info;
    (void)ctx;
    copy_tag();
    for (unsigned i = 0; i < 8; ++i)
        fault_capture[8 + i] = fault_target[i];
    latx_avx_single_write_all(fault_capture, sizeof(fault_capture));
    exit_now(128 + sig);
}

static int install_handlers(void)
{
    struct kernel_sigaction action = {
        fault_handler, SA_SIGINFO | SA_RESTORER, latx_avx_single_rt_sigreturn,
        0,
    };
    return syscall4(SYS_RT_SIGACTION, SIGSEGV, (long)(uintptr_t)&action, 0,
                    8) < 0 ||
           syscall4(SYS_RT_SIGACTION, SIGBUS, (long)(uintptr_t)&action, 0,
                    8) < 0;
}

int latx_avx_single_main(long argc, char **argv)
{
    long mapping;
    uint8_t *page;

    if (argc == 1) {
        latx_avx_single_@MNEMONIC@_run(output, 0);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc != 2 || (!same(argv[1], "xmm-cross-8") &&
                      !same(argv[1], "ymm-cross-16")))
        return 2;
    mapping = syscall6(SYS_MMAP, 0, PAGES * PAGE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping < 0)
        return 3;
    page = (uint8_t *)(uintptr_t)mapping;
    latx_avx_single_fill(page, PAGES * PAGE, UINT64_C(0x91e10da4c3b2f587));
    if (install_handlers() ||
        syscall3(SYS_MPROTECT, (long)(uintptr_t)(page + PAGE), PAGE,
                 PROT_NONE) < 0)
        return 4;
    if (same(argv[1], "xmm-cross-8")) {
        fault_name = "xmm-cross-8";
        fault_target = page + PAGE - 8;
        latx_avx_single_@MNEMONIC@_fault_xmm(fault_target);
    } else {
        fault_name = "ymm-cross-16";
        fault_target = page + PAGE - 16;
        latx_avx_single_@MNEMONIC@_fault_ymm(fault_target);
    }
    return 5;
}
'''


def words(kind):
    if kind == "ps":
        return (
            "    .long 0x7fc01234, 0x7fa00042, 0x00000000, 0x80000000",
            "    .long 0x7f800000, 0xff800000, 0x00000001, 0x80000001",
        )
    return (
        "    .quad 0x7ff8000000000042, 0x7ff0000000000001",
        "    .quad 0x7ff0000000000000, 0xfff0000000000000",
        "    .quad 0x0000000000000001, 0x8000000000000001",
        "    .quad 0x0000000000000000, 0x8000000000000000",
    )


def build_asm(mnemonic):
    kind = "ps" if mnemonic.endswith("ps") else "pd"
    a = words(kind)
    if kind == "ps":
        b = (
            "    .long 0x3f800000, 0xbf800000, 0x7fc05678, 0x7fa00099",
            "    .long 0x00800000, 0x80800000, 0x7f800000, 0xff800000",
        )
    else:
        b = (
            "    .quad 0x3ff0000000000000, 0xbff0000000000000",
            "    .quad 0x7ff8000000000056, 0x7ff0000000000099",
            "    .quad 0x0010000000000000, 0x8010000000000000",
            "    .quad 0x7ff0000000000000, 0xfff0000000000000",
        )
    return (ASM.replace("@MNEMONIC@", mnemonic)
            .replace("@INPUT_A@", "\n".join(a))
            .replace("@INPUT_B@", "\n".join(b)))


def build_c(mnemonic):
    return C.replace("@MNEMONIC@", mnemonic)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for mnemonic in MNEMONICS:
        (args.output_dir / f"latx-avx-single-{mnemonic}.S").write_text(
            build_asm(mnemonic), encoding="ascii")
        (args.output_dir / f"latx-avx-single-{mnemonic}.c").write_text(
            build_c(mnemonic), encoding="ascii")
    manifest = {
        "work_item": "WI-1920",
        "mnemonics": list(MNEMONICS),
        "baseline": "xzy86 native x86",
        "cases_per_mnemonic": 12,
        "record_size": 32,
        "normal_output_size": 384,
        "fault_cases": ["xmm-cross-8", "ymm-cross-16"],
        "coverage": [
            "XMM and YMM register sources",
            "XMM and YMM legal memory source",
            "destination/src1/src2 aliases",
            "both 128-bit lanes for YMM",
            "qNaN sNaN Inf subnormal +0 -0 bit patterns",
            "VEX.128 high half clear",
            "unaligned legal memory source",
            "cross-page XMM/YMM source fault",
        ],
    }
    (args.output_dir / "wi1920-fixture-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print("mnemonics=" + " ".join(MNEMONICS))


if __name__ == "__main__":
    main()
