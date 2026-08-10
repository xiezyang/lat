#!/usr/bin/env python3
"""Generate an independent xzy86 fixture for AVX VMPSADBW."""

from pathlib import Path
import argparse
import json


ASM = r'''/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro SAVE_RESULT offset
    vmovdqu ymmword ptr [rdi + \offset], ymm2
    .endm

    .globl latx_avx_single_vmpsadbw_run
    .type latx_avx_single_vmpsadbw_run, @function
latx_avx_single_vmpsadbw_run:
@XMM_CASES@
@YMM_CASES@
    vzeroupper
    ret

    .globl latx_avx_single_vmpsadbw_fault_xmm
    .type latx_avx_single_vmpsadbw_fault_xmm, @function
latx_avx_single_vmpsadbw_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmpsadbw xmm2, xmm0, xmmword ptr [rdi], 7
    ret

    .globl latx_avx_single_vmpsadbw_fault_ymm
    .type latx_avx_single_vmpsadbw_fault_ymm, @function
latx_avx_single_vmpsadbw_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmpsadbw ymm2, ymm0, ymmword ptr [rdi], 7
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
    CASES = 20, RECORD_SIZE = 32, OUTPUT_SIZE = CASES * RECORD_SIZE,
};

struct kernel_sigaction {
    void (*handler)(int, void *, void *);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

extern void latx_avx_single_vmpsadbw_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vmpsadbw_fault_xmm(uint8_t *);
extern void latx_avx_single_vmpsadbw_fault_ymm(uint8_t *);
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

static void fault_handler(int sig, void *info, void *ctx)
{
    (void)info;
    (void)ctx;
    for (unsigned i = 0; i < 8; ++i)
        fault_capture[i] = fault_name[i] ? (uint8_t)fault_name[i] : 0;
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
        latx_avx_single_vmpsadbw_run(output, 0);
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
    latx_avx_single_fill(page, PAGES * PAGE, UINT64_C(0x62a4c91e7b350d88));
    if (install_handlers() ||
        syscall3(SYS_MPROTECT, (long)(uintptr_t)(page + PAGE), PAGE,
                 PROT_NONE) < 0)
        return 4;
    if (same(argv[1], "xmm-cross-8")) {
        fault_name = "xmm-cross-8";
        fault_target = page + PAGE - 8;
        latx_avx_single_vmpsadbw_fault_xmm(fault_target);
    } else {
        fault_name = "ymm-cross-16";
        fault_target = page + PAGE - 16;
        latx_avx_single_vmpsadbw_fault_ymm(fault_target);
    }
    return 5;
}
'''


def bytes_line(values):
    return "    .byte " + ", ".join(f"0x{value:02x}" for value in values)


def cases(width):
    lines = []
    values = list(range(8)) + [0x08, 0xff]
    for index, imm in enumerate(values):
        offset = index * 32 if width == "xmm" else 320 + index * 32
        reg = "xmm" if width == "xmm" else "ymm"
        alias = index % 4
        lines.append(f"    vmovdqu {reg}0, {reg}word ptr [rip + input_a]")
        lines.append(f"    vmovdqu {reg}1, {reg}word ptr [rip + input_b]")
        if index >= 8:
            lines.append(
                f"    vmpsadbw {reg}2, {reg}0, {reg}word ptr "
                f"[rip + input_b_unaligned], {imm}")
        elif alias == 0:
            lines.append(f"    vmpsadbw {reg}2, {reg}0, {reg}1, {imm}")
        elif alias == 1:
            lines.append(f"    vmpsadbw {reg}0, {reg}0, {reg}1, {imm}")
            lines.append(f"    vmovdqu {reg}2, {reg}0")
        elif alias == 2:
            lines.append(f"    vmpsadbw {reg}1, {reg}0, {reg}1, {imm}")
            lines.append(f"    vmovdqu {reg}2, {reg}1")
        else:
            lines.append(f"    vmpsadbw {reg}0, {reg}0, {reg}0, {imm}")
            lines.append(f"    vmovdqu {reg}2, {reg}0")
        lines.append(f"    SAVE_RESULT {offset}")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    input_a = bytes_line([
        0x00, 0xff, 0x10, 0xef, 0x20, 0xdf, 0x30, 0xcf,
        0x40, 0xbf, 0x50, 0xaf, 0x60, 0x9f, 0x70, 0x8f,
        0x01, 0xfe, 0x11, 0xee, 0x21, 0xde, 0x31, 0xce,
        0x41, 0xbe, 0x51, 0xae, 0x61, 0x9e, 0x71, 0x8e,
    ])
    input_b = bytes_line([
        0xff, 0x00, 0xf0, 0x0f, 0xe0, 0x1f, 0xd0, 0x2f,
        0xc0, 0x3f, 0xb0, 0x4f, 0xa0, 0x5f, 0x90, 0x6f,
        0xfe, 0x01, 0xee, 0x11, 0xde, 0x21, 0xce, 0x31,
        0xbe, 0x41, 0xae, 0x51, 0x9e, 0x61, 0x8e, 0x71,
    ])
    asm = (ASM.replace("@XMM_CASES@", cases("xmm"))
           .replace("@YMM_CASES@", cases("ymm"))
           .replace("@INPUT_A@", input_a)
           .replace("@INPUT_B@", input_b))
    (args.output_dir / "latx-avx-single-vmpsadbw.S").write_text(asm,
                                                                  encoding="ascii")
    (args.output_dir / "latx-avx-single-vmpsadbw.c").write_text(C,
                                                                 encoding="ascii")
    manifest = {
        "work_item": "WI-1922",
        "mnemonic": "vmpsadbw",
        "baseline": "xzy86 native x86",
        "cases": 20,
        "record_size": 32,
        "normal_output_size": 640,
        "imm8_values": [0, 1, 2, 3, 4, 5, 6, 7, 8, 255],
        "fault_cases": ["xmm-cross-8", "ymm-cross-16"],
        "coverage": [
            "XMM and YMM register forms",
            "XMM and YMM memory-source forms",
            "destination/src1/src2 register aliases",
            "byte-pattern SAD results in both YMM lanes",
            "imm8 selector values 0..7 and high-bit variants 0x08/0xff",
            "VEX.128 high half clear",
            "unaligned legal memory source",
            "XMM and YMM cross-page source faults",
        ],
    }
    (args.output_dir / "wi1922-fixture-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print("mnemonic=vmpsadbw imm8=0,1,2,3,4,5,6,7,8,255")


if __name__ == "__main__":
    main()
