#!/usr/bin/env python3
"""Generate independent x86 assembly fixtures for WI-1913."""

from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path


MNEMONICS = (
    "vblendpd", "vblendps", "vblendvpd", "vblendvps", "vpalignr",
    "vpblendd", "vpblendvb", "vpblendw", "vperm2f128", "vperm2i128",
    "vpermd", "vpermilpd", "vpermilps", "vpermpd", "vpermps", "vpermq",
)


def operation(mnemonic: str, width: str, operands: str) -> str:
    return f"    {mnemonic} {width}{operands}"


def cases(mnemonic: str) -> list[tuple[str, str]]:
    if mnemonic in {"vblendpd", "vblendps", "vpblendd", "vpblendw"}:
        return [
            ("xmm", "0, xmm1, xmm2, 0"),
            ("xmm", "0, xmm1, xmm1, 255"),
            ("ymm", "0, ymm1, ymm2, 85"),
            ("ymm", "0, ymm0, ymm2, 170"),
            ("xmm", "0, xmm1, xmmword ptr [rip + input_b], 255"),
            ("ymm", "0, ymm1, ymmword ptr [rip + input_b], 0"),
        ]
    if mnemonic in {"vblendvpd", "vblendvps", "vpblendvb"}:
        return [
            ("xmm", "0, xmm1, xmm2, xmm3"),
            ("xmm", "0, xmm0, xmm2, xmm3"),
            ("ymm", "0, ymm1, ymm2, ymm3"),
            ("ymm", "0, ymm0, ymm2, ymm3"),
            ("xmm", "0, xmm1, xmmword ptr [rip + input_b], xmm3"),
            ("ymm", "0, ymm1, ymmword ptr [rip + input_b], ymm3"),
        ]
    if mnemonic == "vpalignr":
        return [
            ("xmm", "0, xmm1, xmm2, 0"),
            ("xmm", "0, xmm1, xmm2, 15"),
            ("xmm", "0, xmm1, xmm2, 16"),
            ("ymm", "0, ymm1, ymm2, 1"),
            ("ymm", "0, ymm0, ymm2, 31"),
            ("ymm", "0, ymm1, ymmword ptr [rip + input_b], 32"),
        ]
    if mnemonic in {"vperm2f128", "vperm2i128"}:
        return [
            ("ymm", "0, ymm1, ymm2, 0"),
            ("ymm", "0, ymm1, ymm2, 1"),
            ("ymm", "0, ymm0, ymm2, 0x88"),
            ("ymm", "0, ymm1, ymmword ptr [rip + input_b], 0x31"),
        ]
    if mnemonic == "vpermd":
        return [
            ("ymm", "0, ymm1, ymm2"),
            ("ymm", "0, ymm0, ymm2"),
            ("ymm", "0, ymm1, ymm1"),
            ("ymm", "0, ymm1, ymmword ptr [rip + input_b]"),
        ]
    if mnemonic in {"vpermilpd", "vpermilps"}:
        return [
            ("xmm", "0, xmm1, 0"),
            ("xmm", "0, xmm0, 255"),
            ("xmm", "0, xmm1, xmm2"),
            ("ymm", "0, ymm1, 85"),
            ("ymm", "0, ymm0, ymm2"),
            ("ymm", "0, ymm1, ymmword ptr [rip + input_b]"),
        ]
    if mnemonic in {"vpermpd", "vpermq"}:
        return [
            ("ymm", "0, ymm1, 0"),
            ("ymm", "0, ymm1, 27"),
            ("ymm", "0, ymm0, 255"),
            ("ymm", "0, ymm1, ymm1"),
        ]
    if mnemonic == "vpermps":
        return [
            ("ymm", "0, ymm1, ymm2"),
            ("ymm", "0, ymm0, ymm2"),
            ("ymm", "0, ymm1, ymm1"),
            ("ymm", "0, ymm1, ymmword ptr [rip + input_b]"),
        ]
    raise ValueError(mnemonic)


def assembly(mnemonic: str) -> str:
    body: list[str] = []
    for index, (width, operands) in enumerate(cases(mnemonic)):
        body.append(f"    vmovdqu {width}0, {width}word ptr [rip + input_a]")
        if "xmm1" in operands:
            body.append("    vmovdqu xmm1, xmmword ptr [rip + input_b]")
        if "ymm1" in operands:
            body.append("    vmovdqu ymm1, ymmword ptr [rip + input_a]")
        if "xmm2" in operands:
            body.append("    vmovdqu xmm2, xmmword ptr [rip + input_b]")
        if "ymm2" in operands:
            body.append("    vmovdqu ymm2, ymmword ptr [rip + input_b]")
        if "xmm3" in operands:
            body.append("    vmovdqu xmm3, xmmword ptr [rip + mask]")
        if "ymm3" in operands:
            body.append("    vmovdqu ymm3, ymmword ptr [rip + mask]")
        body.append(operation(mnemonic, width, operands))
        body.append(f"    vmovdqu ymmword ptr [rdi + {index * 32}], ymm0")
    return f"""/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .globl latx_avx_single_{mnemonic}_run
    .type latx_avx_single_{mnemonic}_run, @function
latx_avx_single_{mnemonic}_run:
{chr(10).join(body)}
    vzeroupper
    ret

    .section .rodata
    .balign 32
input_a:
    .quad 0x80017fff00010000, 0x7fff80010000ffff
    .quad 0x0123456789abcdef, 0xfedcba9876543210
input_b:
    .quad 0xffff00007fff8000, 0x00018000ffff7fff
    .quad 0x7ff8000000000042, 0x7ff0000000000001
mask:
    .quad 0x8000000080000000, 0x7fffffff00000000
    .quad 0x8000000000000000, 0x000000007fffffff

    .section .note.GNU-stack,"",@progbits
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default=None)
    args = parser.parse_args()
    if args.output is None:
        output = Path(tempfile.mkdtemp(prefix="wi-1913-fixtures-", dir="/tmp"))
    else:
        output = Path(args.output)
        if not str(output).startswith("/tmp/"):
            parser.error("--output must be under /tmp")
        output.mkdir(parents=True, exist_ok=True)
    manifest = {"mnemonics": [], "source": str(output)}
    for mnemonic in MNEMONICS:
        path = output / f"latx-avx-single-{mnemonic}.S"
        path.write_text(assembly(mnemonic), encoding="ascii")
        manifest["mnemonics"].append({
            "mnemonic": mnemonic,
            "source": str(path),
            "case_count": len(cases(mnemonic)),
        })
    (output / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="ascii"
    )
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
