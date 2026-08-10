#!/usr/bin/env python3
"""Generate one independent native fixture for every WI-1842 mnemonic."""

from __future__ import annotations

import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
INTEGRATION = ROOT / "tests/integration"

SCALAR = {
    "vpslld": ("d", 31),
    "vpsllq": ("q", 63),
    "vpsllw": ("w", 15),
    "vpsrad": ("d", 31),
    "vpsraw": ("w", 15),
    "vpsrld": ("d", 31),
    "vpsrlq": ("q", 63),
    "vpsrlw": ("w", 15),
}

VECTOR = {
    "vpsllvd": ("d", 31),
    "vpsllvq": ("q", 63),
    "vpsravd": ("d", 31),
    "vpsrlvd": ("d", 31),
    "vpsrlvq": ("q", 63),
}

BYTE = {
    "vpslldq": "left",
    "vpsrldq": "right",
}


def load(width: str, reg: int, label: str) -> str:
    return f"    vmovdqu {'ymm' if width == 'ymm' else 'xmm'}{reg}, {'ymmword' if width == 'ymm' else 'xmmword'} ptr [rip + {label}]"


def save_macro(mnemonic: str) -> str:
    return f"""    .macro SAVE_{mnemonic.upper()} offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + {mnemonic}_xsave_area]
    movdqu xmm14, xmmword ptr [rip + {mnemonic}_xsave_area + 160 + (\\reg) * 16]
    movdqu xmmword ptr [rdi + \\offset], xmm14
    movdqu xmm14, xmmword ptr [rip + {mnemonic}_xsave_area + 576 + (\\reg) * 16]
    movdqu xmmword ptr [rdi + \\offset + 16], xmm14
    .endm
"""


def scalar_cases(mnemonic: str, suffix: str, maximum: int) -> list[str]:
    cases: list[str] = []
    imm = [0, maximum - 1, maximum, 255]
    for index, count in enumerate(imm):
        dest = 0 if index in (1, 3) else 2
        cases.extend([
            load("xmm", 0, f"{mnemonic}_input_a"),
            f"    {mnemonic} xmm{dest}, xmm0, {count}",
            f"    SAVE_{mnemonic.upper()} {index * 32}, {dest}",
        ])

    variable_cases = [
        ("xmm", "reg", 2, 0),
        ("xmm", "mem", 2, max(0, maximum - 1)),
        ("xmm", "reg", 0, maximum),
        ("xmm", "reg", 1, 255),
        ("ymm", "reg", 2, 0),
        ("ymm", "mem", 2, max(0, maximum - 1)),
        ("ymm", "reg", 0, maximum),
        ("ymm", "reg", 1, 255),
    ]
    for index, (width, count_kind, dest, count) in enumerate(variable_cases, 4):
        load_width = "ymm" if width == "ymm" else "xmm"
        cases.extend([
            load(width, 0, f"{mnemonic}_input_a"),
        ])
        if count_kind == "mem":
            count_operand = (
                f"{'xmmword' if width == 'xmm' else 'xmmword'} ptr "
                f"[rip + {mnemonic}_count_{count}]"
            )
            cases.append(
                f"    {mnemonic} {width}{dest}, {width}0, {count_operand}"
            )
        else:
            cases.append(load(load_width if width == "ymm" else "xmm", 1,
                              f"{mnemonic}_count_{count}"))
            count_reg = "xmm1"
            cases.append(
                f"    {mnemonic} {width}{dest}, {width}0, {count_reg}"
            )
        cases.append(f"    SAVE_{mnemonic.upper()} {index * 32}, {dest}")
    return cases


def vector_cases(mnemonic: str, suffix: str, maximum: int) -> list[str]:
    cases: list[str] = []
    variable_cases = [
        ("xmm", "reg", 2, 0),
        ("xmm", "mem", 2, max(0, maximum - 1)),
        ("xmm", "reg", 0, maximum),
        ("xmm", "reg", 1, 255),
        ("ymm", "reg", 2, 0),
        ("ymm", "mem", 2, max(0, maximum - 1)),
        ("ymm", "reg", 0, maximum),
        ("ymm", "reg", 1, 255),
    ]
    for index, (width, count_kind, dest, count) in enumerate(variable_cases):
        cases.extend([
            load(width, 0, f"{mnemonic}_input_a"),
        ])
        if count_kind == "mem":
            operand = "ymmword" if width == "ymm" else "xmmword"
            cases.append(
                f"    {mnemonic} {width}{dest}, {width}0, {operand} ptr "
                f"[rip + {mnemonic}_count_{count}]"
            )
        else:
            cases.append(load(width, 1, f"{mnemonic}_count_{count}"))
            cases.append(f"    {mnemonic} {width}{dest}, {width}0, {width}1")
        cases.append(f"    SAVE_{mnemonic.upper()} {index * 32}, {dest}")
    return cases


def byte_cases(mnemonic: str, direction: str) -> list[str]:
    cases: list[str] = []
    counts = [0, 1, 15, 16, 255, 0, 15, 16]
    for index, count in enumerate(counts):
        width = "xmm" if index < 4 else "ymm"
        reg = 2 if index % 2 == 0 else 0
        cases.extend([
            load(width, 0, f"{mnemonic}_input_a"),
            f"    {mnemonic} {width}{reg}, {width}0, {count}",
            f"    SAVE_{mnemonic.upper()} {index * 32}, {reg}",
        ])
    return cases


def assembly(mnemonic: str) -> str:
    body: list[str]
    if mnemonic in SCALAR:
        suffix, maximum = SCALAR[mnemonic]
        body = scalar_cases(mnemonic, suffix, maximum)
    elif mnemonic in VECTOR:
        suffix, maximum = VECTOR[mnemonic]
        body = vector_cases(mnemonic, suffix, maximum)
    else:
        body = byte_cases(mnemonic, BYTE[mnemonic])

    counts = []
    if mnemonic in SCALAR or mnemonic in VECTOR:
        maximum = (SCALAR | VECTOR)[mnemonic][1]
        for count in [0, max(0, maximum - 1), maximum, 255]:
            counts.append(
                f"{mnemonic}_count_{count}: .quad {count}, {count}, {count}, {count}"
            )
    return f"""/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
{save_macro(mnemonic)}
    .globl latx_avx_single_{mnemonic}_run
    .type latx_avx_single_{mnemonic}_run, @function
latx_avx_single_{mnemonic}_run:
{chr(10).join(body)}
    ret

    .section .data
    .balign 32
{mnemonic}_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
{chr(10).join(counts)}
    .balign 64
{mnemonic}_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
"""


def c_source(mnemonic: str) -> str:
    output_size = 512 if mnemonic in SCALAR else 256
    return f"""/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {{ LATX_AVX_SINGLE_{mnemonic.upper()}_OUTPUT_SIZE = {output_size} }};
extern void latx_avx_single_{mnemonic}_run(uint8_t *output);

static uint8_t output[LATX_AVX_SINGLE_{mnemonic.upper()}_OUTPUT_SIZE];

int latx_avx_single_main(long argc, char **argv)
{{
    (void)argc;
    (void)argv;
    latx_avx_single_{mnemonic}_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}}
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=INTEGRATION)
    args = parser.parse_args()
    mnemonics = sorted(SCALAR | VECTOR | BYTE)
    for mnemonic in mnemonics:
        (args.output_dir / f"latx-avx-single-{mnemonic}.S").write_text(
            assembly(mnemonic), encoding="utf-8"
        )
        (args.output_dir / f"latx-avx-single-{mnemonic}.c").write_text(
            c_source(mnemonic), encoding="utf-8"
        )
    print(f"generated={len(mnemonics)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
