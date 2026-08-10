#!/usr/bin/env python3
"""Generate independent native fixtures for the WI-1843 instruction set."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


MNEMONICS = (
    "vpackssdw", "vpacksswb", "vpackusdw", "vpackuswb",
    "vpunpckhbw", "vpunpckhdq", "vpunpckhqdq", "vpunpckhwd",
    "vpunpcklbw", "vpunpckldq", "vpunpcklqdq", "vpunpcklwd",
    "vshufpd", "vshufps",
)
SHUFFLES = {"vshufpd", "vshufps"}
PRESERVED_FIXTURES = {"vpunpcklqdq"}
ROOT = Path(__file__).resolve().parents[2]
INTEGRATION = ROOT / "tests/integration"
MANIFEST = INTEGRATION / "latx-avx-opt-only-manifest.json"


def load(width: str, reg: int, label: str) -> str:
    word = "ymmword" if width == "ymm" else "xmmword"
    return f"    vmovdqu {width}{reg}, {word} ptr [rip + {label}]"


def operation(mnemonic: str, width: str, dest: int, src1: int,
              src2: str, imm: int | None) -> str:
    text = f"    {mnemonic} {width}{dest}, {width}{src1}, {src2}"
    if imm is not None:
        text += f", {imm}"
    return text


def cases(mnemonic: str) -> list[tuple[str, int, int, str, int | None]]:
    if mnemonic in SHUFFLES:
        immediates = [0, 1, 3, 15, 0x55, 0xaa, 0xff, 5, 0, 0xff, 1, 0xf]
        return [
            ("xmm", 2, 0, "xmm1", immediates[0]),
            ("xmm", 0, 0, "xmm1", immediates[1]),
            ("xmm", 1, 0, "xmm1", immediates[2]),
            ("xmm", 0, 0, "xmm0", immediates[3]),
            ("ymm", 2, 0, "ymm1", immediates[4]),
            ("ymm", 0, 0, "ymm1", immediates[5]),
            ("ymm", 1, 0, "ymm1", immediates[6]),
            ("ymm", 0, 0, "ymm0", immediates[7]),
            ("xmm", 2, 0, "xmmword ptr [rip + input_b]", immediates[8]),
            ("xmm", 0, 0, "xmmword ptr [rip + input_b]", immediates[9]),
            ("ymm", 2, 0, "ymmword ptr [rip + input_b]", immediates[10]),
            ("ymm", 0, 0, "ymmword ptr [rip + input_b]", immediates[11]),
        ]
    result = []
    for width in ("xmm", "ymm"):
        result.extend([
            (width, 2, 0, f"{width}1", None),
            (width, 0, 0, f"{width}1", None),
            (width, 1, 0, f"{width}1", None),
            (width, 0, 0, f"{width}0", None),
        ])
    result.extend([
        ("xmm", 2, 0, "xmmword ptr [rip + input_b]", None),
        ("xmm", 0, 0, "xmmword ptr [rip + input_b]", None),
        ("ymm", 2, 0, "ymmword ptr [rip + input_b]", None),
        ("ymm", 0, 0, "ymmword ptr [rip + input_b]", None),
    ])
    return result


def assembly(mnemonic: str) -> str:
    body = []
    for index, (width, dest, src1, src2, imm) in enumerate(cases(mnemonic)):
        body.append(load(width, 0, "input_a"))
        if src2 in {"xmm1", "ymm1"}:
            body.append(load(width, 1, "input_b"))
        body.append(operation(mnemonic, width, dest, src1, src2, imm))
        body.append(f"    vmovdqu ymmword ptr [rdi + {index * 32}], ymm{dest}")
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

    .section .note.GNU-stack,"",@progbits
"""


def c_source(mnemonic: str) -> str:
    return f"""/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {{ OUTPUT_SIZE = 12 * 32 }};
static uint8_t output[OUTPUT_SIZE];
extern void latx_avx_single_{mnemonic}_run(uint8_t *);

int latx_avx_single_main(long argc, char **argv)
{{
    (void)argc;
    (void)argv;
    latx_avx_single_{mnemonic}_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}}
"""


def update_manifest(manifest_path: Path) -> None:
    manifest = json.loads(manifest_path.read_text())
    wanted = set(MNEMONICS)
    for entry in manifest["entries"]:
        if entry["mnemonic"] not in wanted:
            continue
        mnemonic = entry["mnemonic"]
        if mnemonic in PRESERVED_FIXTURES:
            entry["runner"] = "tests/integration/test-latx-avx-single-vpunpcklqdq.sh"
            continue
        entry["coverage_status"] = "existing_fixture"
        entry["manual_template_required"] = False
        entry["source_files"] = [
            f"tests/integration/latx-avx-single-{mnemonic}.S",
            f"tests/integration/latx-avx-single-{mnemonic}.c",
        ]
        entry["runner"] = "tests/integration/test-latx-avx-wi1843-fixtures.sh"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=INTEGRATION)
    parser.add_argument("--manifest", type=Path, default=MANIFEST)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for mnemonic in MNEMONICS:
        if mnemonic in PRESERVED_FIXTURES:
            continue
        (args.output_dir / f"latx-avx-single-{mnemonic}.S").write_text(assembly(mnemonic))
        (args.output_dir / f"latx-avx-single-{mnemonic}.c").write_text(c_source(mnemonic))
    update_manifest(args.manifest)
    print(f"PASS WI-1843 generated fixtures: count={len(MNEMONICS) - len(PRESERVED_FIXTURES)}; preserved=1")


if __name__ == "__main__":
    main()
