#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

python3 - "$root" <<'PY'
import sys
from pathlib import Path

root = Path(sys.argv[1])
fixture_dir = root / "tests/integration"
mnemonics = (
    "vpcmpeqb", "vpcmpeqw", "vpcmpeqd", "vpcmpeqq",
    "vpcmpgtb", "vpcmpgtw", "vpcmpgtd", "vpcmpgtq",
)
asm_template = (fixture_dir / "latx-avx-single-vintcmp-template.S").read_text()
c_template = (fixture_dir / "latx-avx-single-vintcmp-template.c").read_text()
builder = (fixture_dir / "build-latx-avx-wi1840-cmp-xzy86.sh").read_text()

assert "LATX_VINTCMP_MNEMONIC xmm0, xmm1, xmm2" in asm_template
assert "LATX_VINTCMP_MNEMONIC ymm0, ymm1, ymm2" in asm_template
assert "LATX_VINTCMP_MNEMONIC xmm0, xmm0, xmm2" in asm_template
assert "LATX_VINTCMP_MNEMONIC ymm0, ymm0, ymm2" in asm_template
assert "LATX_VINTCMP_MNEMONIC xmm0, xmm1, xmm0" in asm_template
assert "LATX_VINTCMP_MNEMONIC ymm0, ymm1, ymm0" in asm_template
assert "xmmword ptr [rsi]" in asm_template
assert "ymmword ptr [rsi]" in asm_template
assert "xsave64" in asm_template
assert ".quad 0x0102030405060708" in asm_template
assert "LATX_VINTCMP_RUN" in c_template
assert "latx_avx_single_fill" in c_template
assert "LATX_VINTCMP_RUN(latx_vintcmp_output, latx_vintcmp_page, 0)" in c_template
assert "latx-avx-single-vintcmp-template.S" in builder
assert "latx-avx-single-vintcmp-template.c" in builder

for mnemonic in mnemonics:
    if mnemonic == "vpcmpeqq":
        asm = (fixture_dir / "latx-avx-single-vpcmpeqq.S").read_text()
        c = (fixture_dir / "latx-avx-single-vpcmpeqq.c").read_text()
        assert "vpcmpeqq ymm0, ymm1, ymm2" in asm
        assert "vpcmpeqq xmm0, xmm1, xmm2" in asm
        assert "latx_avx_single_vpcmpeqq_run" in c
        continue
    asm = (fixture_dir / f"latx-avx-single-{mnemonic}.S").read_text()
    c = (fixture_dir / f"latx-avx-single-{mnemonic}.c").read_text()
    symbol = f"latx_avx_single_{mnemonic}"
    assert f"#define LATX_VINTCMP_MNEMONIC {mnemonic}" in asm
    assert f"#define LATX_VINTCMP_SYMBOL_PREFIX {symbol}" in asm
    assert f"#define LATX_VINTCMP_PREFIX {mnemonic}" in c
    assert f"#define LATX_VINTCMP_SYMBOL_PREFIX {symbol}" in c
    assert asm.count("#include \"latx-avx-single-vintcmp-template.S\"") == 1
    assert c.count("#include \"latx-avx-single-vintcmp-template.c\"") == 1
    assert "vpcmpeqq" not in asm + c

print("PASS WI-1840 eight independent comparison fixtures and ABI entries")
PY
