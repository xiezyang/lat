#!/usr/bin/env python3
"""Generate independent native x86 fixtures for WI-1916 AES/CLMUL."""

from pathlib import Path
import argparse
from dataclasses import dataclass
from typing import Optional

MNEMONICS = (
    "vaesdec vaesdeclast vaesenc vaesenclast vaesimc "
    "vaeskeygenassist vpclmulqdq"
).split()
XMM_ONLY = {"vaesimc", "vaeskeygenassist"}
IMMEDIATE = {"vaeskeygenassist", "vpclmulqdq"}


@dataclass(frozen=True)
class Case:
    width: int
    dest: int
    src1: int
    src2: Optional[int] = None
    memory: bool = False
    imm: Optional[int] = None


def _round_cases():
    patterns = (
        (2, 0, 1, False),
        (0, 0, 1, False),
        (1, 0, 1, False),
        (2, 0, None, True),
        (15, 15, 1, False),
        (15, 0, 15, False),
    )
    return [Case(width, dest, src1, src2, memory)
            for width in (128, 256)
            for dest, src1, src2, memory in patterns]


def _xmm_only_cases(mnemonic):
    if mnemonic == "vaesimc":
        patterns = (
            (2, 0, False), (2, 0, True), (0, 0, False),
            (15, 15, False), (1, 0, False), (15, 0, True),
            (2, 1, False), (0, 0, True), (15, 15, False),
            (1, 0, False), (2, 0, True), (0, 0, False),
        )
        return [Case(128, dest, src1, memory=memory)
                for dest, src1, memory in patterns]

    patterns = (
        (2, 0, False, 0), (2, 0, True, 1), (0, 0, False, 27),
        (15, 15, False, 255), (1, 0, False, 0), (15, 0, True, 1),
        (2, 1, False, 27), (0, 0, True, 255), (15, 15, False, 0),
        (1, 0, False, 1), (2, 0, True, 27), (0, 0, False, 255),
    )
    return [Case(128, dest, src1, memory=memory, imm=imm)
            for dest, src1, memory, imm in patterns]


def _vpclmul_cases():
    patterns = (
        (2, 0, 1, False, 0), (0, 0, 1, False, 1),
        (1, 0, 1, False, 16), (2, 0, None, True, 255),
        (15, 15, 1, False, 0), (15, 0, 15, False, 1),
    )
    return [Case(width, dest, src1, src2, memory, imm)
            for width in (128, 256)
            for dest, src1, src2, memory, imm in patterns]


def cases_for(mnemonic):
    if mnemonic in XMM_ONLY:
        return _xmm_only_cases(mnemonic)
    if mnemonic == "vpclmulqdq":
        return _vpclmul_cases()
    return _round_cases()


def _reg(vector, index):
    return f"{vector}{index}"


def op(mnemonic, case):
    vector = "xmm" if case.width == 128 else "ymm"
    mem = "xmmword ptr [rip + input_b]" if vector == "xmm" else "ymmword ptr [rip + input_b]"
    dest = _reg(vector, case.dest)
    src1 = _reg(vector, case.src1)
    if mnemonic == "vaesimc":
        source = mem if case.memory else src1
        return f"    {mnemonic} {dest}, {source}\n"
    if mnemonic == "vaeskeygenassist":
        source = mem if case.memory else src1
        return f"    {mnemonic} {dest}, {source}, {case.imm}\n"
    src2 = mem if case.memory else _reg(vector, case.src2)
    if mnemonic == "vpclmulqdq":
        return f"    {mnemonic} {dest}, {src1}, {src2}, {case.imm}\n"
    return f"    {mnemonic} {dest}, {src1}, {src2}\n"


def setup(case):
    vector = "xmm" if case.width == 128 else "ymm"
    loads = {0: "input_a", 1: "input_b"}
    if case.src1 not in loads:
        loads[case.src1] = "input_a"
    if case.src2 is not None and not case.memory and case.src2 not in loads:
        loads[case.src2] = "input_b"
    return [f"    vmovdqu {_reg(vector, index)}, {vector}word ptr [rip + {name}]\n"
            for index, name in sorted(loads.items())]


def fault_op(mnemonic, vector):
    mem = "xmmword ptr [rdi]" if vector == "xmm" else "ymmword ptr [rdi]"
    if mnemonic == "vaesimc":
        return f"    {mnemonic} xmm15, {mem}\n    ret\n"
    if mnemonic == "vaeskeygenassist":
        return f"    {mnemonic} xmm15, {mem}, 255\n    ret\n"
    if mnemonic == "vpclmulqdq":
        return f"    {mnemonic} {vector}15, {vector}0, {mem}, 255\n    ret\n"
    return f"    {mnemonic} {vector}15, {vector}0, {mem}\n    ret\n"


def asm(mnemonic):
    lines = [
        "/* SPDX-License-Identifier: GPL-2.0-only */\n\n",
        "    .intel_syntax noprefix\n    .text\n",
        f"    .globl latx_avx_single_{mnemonic}_run\n",
        f"    .type latx_avx_single_{mnemonic}_run, @function\n",
        f"latx_avx_single_{mnemonic}_run:\n",
    ]
    cases = cases_for(mnemonic)
    assert len(cases) == 12
    for case_index, case in enumerate(cases):
        vector = "xmm" if case.width == 128 else "ymm"
        lines += setup(case)
        lines += [
            op(mnemonic, case),
            f"    vmovdqu ymmword ptr [rdi + {case_index * 32}], ymm{case.dest}\n",
        ]
    lines += [
        "    vzeroupper\n    ret\n\n",
        f"    .globl latx_avx_single_{mnemonic}_fault_xmm\n",
        f"    .type latx_avx_single_{mnemonic}_fault_xmm, @function\n",
        f"latx_avx_single_{mnemonic}_fault_xmm:\n",
        "    vmovdqu xmm0, xmmword ptr [rip + input_a]\n",
        fault_op(mnemonic, "xmm"),
    ]
    if mnemonic not in XMM_ONLY:
        lines += [
            f"    .globl latx_avx_single_{mnemonic}_fault_ymm\n",
            f"    .type latx_avx_single_{mnemonic}_fault_ymm, @function\n",
            f"latx_avx_single_{mnemonic}_fault_ymm:\n",
            "    vmovdqu ymm0, ymmword ptr [rip + input_a]\n",
            fault_op(mnemonic, "ymm"),
        ]
    lines += [
        "\n    .section .rodata\n    .balign 32\n",
        "input_a:\n",
        "    .quad 0x0001020304050607, 0x08090a0b0c0d0e0f\n",
        "    .quad 0x1011121314151617, 0x18191a1b1c1d1e1f\n",
        "input_b:\n",
        "    .quad 0xffeeddccbbaa9988, 0x7766554433221100\n",
        "    .quad 0x0123456789abcdef, 0xfedcba9876543210\n",
        "\n    .section .note.GNU-stack,\"\",@progbits\n",
    ]
    return "".join(lines)


def c_source(mnemonic):
    has_ymm = mnemonic not in XMM_ONLY
    decl_ymm = f"extern void latx_avx_single_{mnemonic}_fault_ymm(uint8_t *);\n" if has_ymm else ""
    run_ymm = (f"    if (streq(argv[1], \"fault-ymm\")) {{\n"
               f"        latx_avx_single_{mnemonic}_fault_ymm(page + 4096 - 31);\n"
               "        return 90;\n    }\n") if has_ymm else ""
    return f'''/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"
enum {{ OUTPUT_SIZE = 12 * 32, SYS_MMAP = 9, SYS_MPROTECT = 10 }};
static uint8_t output[OUTPUT_SIZE];
extern void latx_avx_single_{mnemonic}_run(uint8_t *);
extern void latx_avx_single_{mnemonic}_fault_xmm(uint8_t *);
{decl_ymm}
static inline long syscall6(long n, long a0, long a1, long a2, long a3, long a4, long a5)
{{
    register long rax __asm__("rax") = n, rdi __asm__("rdi") = a0;
    register long rsi __asm__("rsi") = a1, rdx __asm__("rdx") = a2;
    register long r10 __asm__("r10") = a3, r8 __asm__("r8") = a4, r9 __asm__("r9") = a5;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx),
                     "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return rax;
}}
static int streq(const char *a, const char *b)
{{ while (*a != '\\0' && *a == *b) {{ ++a; ++b; }} return *a == *b; }}
int latx_avx_single_main(long argc, char **argv)
{{
    if (argc == 1 || (argc == 2 && streq(argv[1], "reference"))) {{
        latx_avx_single_{mnemonic}_run(output);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }}
    if (argc != 2 || (!streq(argv[1], "fault-xmm") && !streq(argv[1], "fault-ymm"))) return 72;
    long mapping = syscall6(SYS_MMAP, 0, 8192, 3, 0x22, -1, 0);
    if (mapping < 0) return 70;
    uint8_t *page = (uint8_t *)(uintptr_t)mapping;
    if (syscall6(SYS_MPROTECT, (long)(uintptr_t)(page + 4096), 4096, 0, 0, 0, 0) < 0) return 71;
    if (streq(argv[1], "fault-xmm")) {{
        latx_avx_single_{mnemonic}_fault_xmm(page + 4096 - 16 + 1);
        return 90;
    }}
{run_ymm}    return 72;
}}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()
    output = Path(args.output_dir)
    for mnemonic in MNEMONICS:
        (output / f"latx-avx-single-{mnemonic}.S").write_text(asm(mnemonic), encoding="ascii")
        (output / f"latx-avx-single-{mnemonic}.c").write_text(c_source(mnemonic), encoding="ascii")


if __name__ == "__main__":
    main()
