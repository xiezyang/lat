#!/usr/bin/env python3
"""Generate one native x86 fixture for every WI-1914 mnemonic."""

from pathlib import Path
import argparse


MNEMONICS = (
    "vaddpd vaddps vaddsd vaddss vaddsubpd vaddsubps "
    "vandnpd vandnps vandpd vandps vdivpd vdivps vdivss vdppd vdpps "
    "vhaddpd vhaddps vhsubpd vhsubps vmaxpd vmaxps vmaxsd vmaxss "
    "vminpd vminps vminsd vminss vmulpd vmulps vmulss vorpd vorps "
    "vrcpps vrcpss vrsqrtps vrsqrtss vsqrtpd vsqrtps vsqrtsd vsqrtss "
    "vsubpd vsubps vsubsd vsubss vxorpd vxorps"
).split()

PACKED_BINARY = {
    "vaddpd", "vaddps", "vaddsubpd", "vaddsubps", "vandnpd", "vandnps",
    "vandpd", "vandps", "vdivpd", "vdivps", "vdppd", "vdpps",
    "vhaddpd", "vhaddps", "vhsubpd", "vhsubps", "vmaxpd", "vmaxps",
    "vminpd", "vminps", "vmulpd", "vmulps", "vorpd", "vorps",
    "vsubpd", "vsubps", "vxorpd", "vxorps",
}
PACKED_UNARY = {"vrcpps", "vrsqrtps", "vsqrtpd", "vsqrtps"}
DOT = {"vdppd", "vdpps"}
PACKED_XMM_ONLY = {"vdppd"}
SCALAR_BINARY = {
    "vaddsd", "vaddss", "vdivss", "vmaxsd", "vmaxss", "vminsd",
    "vminss", "vmulss", "vsubsd", "vsubss", "vrcpss", "vrsqrtss",
}
SCALAR_UNARY = {"vsqrtsd", "vsqrtss"}


def scalar_size(mnemonic):
    return 8 if mnemonic.endswith("sd") else 4


def memory_type(mnemonic, width, scalar=False):
    if width == 256:
        return "ymmword ptr"
    if not scalar and (mnemonic in PACKED_BINARY or mnemonic in PACKED_UNARY):
        return "xmmword ptr"
    if mnemonic.endswith("pd") or mnemonic.endswith("sd"):
        return "qword ptr"
    return "dword ptr"


def normal_op(mnemonic, width, case):
    vector = "ymm" if width == 256 else "xmm"
    memory = memory_type(mnemonic, width)
    imm = (0, 255, 85, 170)[case % 4]

    if mnemonic in PACKED_BINARY:
        if case % 6 == 0:
            operands = f"{vector}2, {vector}0, {vector}1"
        elif case % 6 == 1:
            operands = f"{vector}0, {vector}0, {vector}1"
        elif case % 6 == 2:
            operands = f"{vector}1, {vector}0, {vector}1"
        elif case % 6 == 3:
            operands = f"{vector}2, {vector}0, {memory} [rip + input_b]"
        elif case % 6 == 4:
            operands = f"{vector}15, {vector}15, {vector}1"
        else:
            operands = f"{vector}15, {vector}0, {vector}15"
        if mnemonic in DOT:
            operands += f", {imm}"
        return f"    {mnemonic} {operands}\n"

    if mnemonic in PACKED_UNARY:
        if case % 4 == 0:
            operands = f"{vector}2, {vector}0"
        elif case % 4 == 1:
            operands = f"{vector}0, {vector}0"
        elif case % 4 == 2:
            operands = f"{vector}15, {vector}15"
        else:
            operands = f"{vector}2, {memory} [rip + input_b]"
        return f"    {mnemonic} {operands}\n"

    if mnemonic in SCALAR_BINARY:
        memory = memory_type(mnemonic, 128, scalar=True)
        if case % 5 == 0:
            operands = "xmm2, xmm0, xmm1"
        elif case % 5 == 1:
            operands = "xmm0, xmm0, xmm1"
        elif case % 5 == 2:
            operands = "xmm1, xmm0, xmm1"
        elif case % 5 == 3:
            operands = f"xmm2, xmm0, {memory} [rip + input_b]"
        else:
            operands = "xmm15, xmm15, xmm1"
        return f"    {mnemonic} {operands}\n"

    if mnemonic in SCALAR_UNARY:
        memory = memory_type(mnemonic, 128, scalar=True)
        if case % 4 == 0:
            operands = "xmm2, xmm0, xmm1"
        elif case % 4 == 1:
            operands = "xmm0, xmm0, xmm1"
        elif case % 4 == 2:
            operands = "xmm15, xmm15, xmm1"
        else:
            operands = f"xmm2, xmm0, {memory} [rip + input_b]"
        return f"    {mnemonic} {operands}\n"

    raise ValueError(mnemonic)


def fault_op(mnemonic, width):
    vector = "ymm" if width == 256 else "xmm"
    memory = memory_type(mnemonic, width)
    if mnemonic in PACKED_BINARY:
        operands = f"{vector}15, {vector}0, {memory} [rdi]"
        if mnemonic in DOT:
            operands += ", 255"
    elif mnemonic in PACKED_UNARY:
        operands = f"{vector}15, {memory} [rdi]"
    elif mnemonic in SCALAR_BINARY:
        operands = f"xmm15, xmm0, {memory_type(mnemonic, 128, scalar=True)} [rdi]"
    elif mnemonic in SCALAR_UNARY:
        operands = (f"xmm15, xmm0, "
                    f"{memory_type(mnemonic, 128, scalar=True)} [rdi]")
    else:
        raise ValueError(mnemonic)
    return f"    {mnemonic} {operands}\n    ret\n"


def fixture_asm(mnemonic):
    packed = mnemonic in PACKED_BINARY or mnemonic in PACKED_UNARY
    lines = [
        "/* SPDX-License-Identifier: GPL-2.0-only */\n",
        "\n    .intel_syntax noprefix\n    .text\n",
        f"    .globl latx_avx_single_{mnemonic}_run\n",
        f"    .type latx_avx_single_{mnemonic}_run, @function\n",
        f"latx_avx_single_{mnemonic}_run:\n",
    ]

    for case in range(12):
        width = (128 if not packed or case < 6 or mnemonic in PACKED_XMM_ONLY
                 else 256)
        vector = "ymm" if width == 256 else "xmm"
        if packed:
            dest = (0 if case % 6 == 1 else
                    1 if case % 6 == 2 else
                    15 if case % 6 >= 4 else 2)
        elif mnemonic in SCALAR_BINARY:
            dest = (0 if case % 5 == 1 else
                    1 if case % 5 == 2 else
                    15 if case % 5 == 4 else 2)
        else:
            dest = (0 if case % 4 == 1 else
                    15 if case % 4 == 2 else 2)
        lines.extend([
            f"    vmovdqu {vector}0, {vector}word ptr [rip + input_a]\n",
            f"    vmovdqu {vector}1, {vector}word ptr [rip + input_b]\n",
            normal_op(mnemonic, width, case),
            f"    vmovdqu ymmword ptr [rdi + {case * 32}], ymm{dest}\n",
        ])
    lines.extend([
        "    vzeroupper\n    ret\n\n",
        f"    .globl latx_avx_single_{mnemonic}_fault_xmm\n",
        f"    .type latx_avx_single_{mnemonic}_fault_xmm, @function\n",
        f"latx_avx_single_{mnemonic}_fault_xmm:\n",
        "    vmovdqu xmm0, xmmword ptr [rip + input_a]\n",
        "    vmovdqu xmm1, xmmword ptr [rip + input_b]\n",
        fault_op(mnemonic, 128),
    ])
    if packed and mnemonic not in PACKED_XMM_ONLY:
        lines.extend([
            f"    .globl latx_avx_single_{mnemonic}_fault_ymm\n",
            f"    .type latx_avx_single_{mnemonic}_fault_ymm, @function\n",
            f"latx_avx_single_{mnemonic}_fault_ymm:\n",
            "    vmovdqu ymm0, ymmword ptr [rip + input_a]\n",
            "    vmovdqu ymm1, ymmword ptr [rip + input_b]\n",
            fault_op(mnemonic, 256),
        ])
    lines.extend([
        "\n    .section .rodata\n    .balign 32\n",
        "input_a:\n",
        "    .quad 0x0000000080000000, 0x8000000000000000\n",
        "    .quad 0x7ff8000000000042, 0x7ff0000000000001\n",
        "input_b:\n",
        "    .quad 0x0000000000000000, 0x8000000000000000\n",
        "    .quad 0x0000000000000001, 0x7ff0000000000000\n",
        "\n    .section .note.GNU-stack,\"\",@progbits\n",
    ])
    return "".join(lines)


def fixture_c(mnemonic):
    packed = mnemonic in PACKED_BINARY or mnemonic in PACKED_UNARY
    has_ymm = packed and mnemonic not in PACKED_XMM_ONLY
    fault_width = "16" if packed else str(scalar_size(mnemonic))
    fault_ymm = ("\nextern void latx_avx_single_%s_fault_ymm(uint8_t *);" % mnemonic
                 if has_ymm else "")
    fault_ymm_case = "" if not has_ymm else (
        "    if (streq(argv[1], \"fault-ymm\")) {\n"
        f"        latx_avx_single_{mnemonic}_fault_ymm(page + 4096 - 31);\n"
        "        return 90;\n"
        "    }\n"
    )
    return f'''/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {{ OUTPUT_SIZE = 12 * 32 }};
enum {{ SYS_MMAP = 9, SYS_MPROTECT = 10, PROT_READ = 1, PROT_WRITE = 2 }};
static uint8_t output[OUTPUT_SIZE];
extern void latx_avx_single_{mnemonic}_run(uint8_t *);
extern void latx_avx_single_{mnemonic}_fault_xmm(uint8_t *);
{fault_ymm}

static inline long syscall6(long number, long a0, long a1, long a2,
                            long a3, long a4, long a5)
{{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = a0;
    register long rsi __asm__("rsi") = a1;
    register long rdx __asm__("rdx") = a2;
    register long r10 __asm__("r10") = a3;
    register long r8 __asm__("r8") = a4;
    register long r9 __asm__("r9") = a5;
    __asm__ volatile("syscall" : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10),
                       "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return rax;
}}

static int streq(const char *left, const char *right)
{{
    while (*left != '\\0' && *left == *right) {{ ++left; ++right; }}
    return *left == *right;
}}

int latx_avx_single_main(long argc, char **argv)
{{
    if (argc == 1 || (argc == 2 && streq(argv[1], "reference"))) {{
        latx_avx_single_{mnemonic}_run(output);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }}
    if (argc != 2 || (!streq(argv[1], "fault-xmm") &&
                      !streq(argv[1], "fault-ymm"))) return 72;
    long mapping = syscall6(SYS_MMAP, 0, 8192, PROT_READ | PROT_WRITE,
                             0x22, -1, 0);
    if (mapping < 0) return 70;
    uint8_t *page = (uint8_t *)(uintptr_t)mapping;
    if (syscall6(SYS_MPROTECT, (long)(uintptr_t)(page + 4096), 4096,
                 0, 0, 0, 0) < 0) return 71;
    if (streq(argv[1], "fault-xmm")) {{
        latx_avx_single_{mnemonic}_fault_xmm(page + 4096 - {fault_width} + 1);
        return 90;
    }}
{fault_ymm_case}    return 72;
}}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()
    root = Path(args.output_dir)
    for mnemonic in MNEMONICS:
        (root / f"latx-avx-single-{mnemonic}.S").write_text(
            fixture_asm(mnemonic), encoding="ascii")
        (root / f"latx-avx-single-{mnemonic}.c").write_text(
            fixture_c(mnemonic), encoding="ascii")
    (root / "latx-avx-wi1914-mnemonics.txt").write_text(
        "\n".join(MNEMONICS) + "\n", encoding="ascii")


if __name__ == "__main__":
    main()
