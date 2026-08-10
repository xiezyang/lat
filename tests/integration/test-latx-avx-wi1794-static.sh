#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
python3 - "$root" <<'PY'
import sys
from pathlib import Path

root = Path(sys.argv[1])
source = (root / "target/i386/latx/translator/tr-avx.c").read_text()
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
header = (root / "target/i386/latx/include/translate.h").read_text()

def function_body(marker):
    start = source.index(marker)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[start:pos + 1]
    raise AssertionError(marker)

context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]

for name in ("pand", "pandn", "por", "pxor"):
    opcode = "V" + name.upper()
    lsx = function_body(f"bool translate_v{name}_lsx")
    assert f"bool translate_v{name}(IR1_INST * pir1)" in source
    assert f"TRANS_FUNC_GEN({opcode}, v{name})" in dispatch
    assert f"TRANS_FUNC_DEF(v{name}_lsx)" in header
    assert "la_xv" not in lsx
    assert "option_enable_lasx" not in lsx
    assert f"translate_register_lsx(dt_X86_INS_{opcode}, translate_v{name}_lsx);" in context

for mnemonic in ("vpand", "vpor", "vpxor"):
    assert (root / f"tests/integration/latx-avx-single-{mnemonic}.S").is_file()
    assert (root / f"tests/integration/latx-avx-single-{mnemonic}.c").is_file()

vpandn = root / "tests/integration/wi1794-vpandn-fixtures"
assert (vpandn / "latx-avx-single-vpandn.S").is_file()
assert (vpandn / "latx-avx-single-vpandn.c").is_file()

vpxor_asm = (root / "tests/integration/latx-avx-single-vpxor.S").read_text()
vpxor_c = (root / "tests/integration/latx-avx-single-vpxor.c").read_text()
for text in ("latx_avx_single_vpxor_fault_xmm", "latx_avx_single_vpxor_fault_ymm",
             "vpxor_zero", "vpxor_ones", "vpxor_sign", "vpxor_random"):
    assert text in vpxor_asm or text in vpxor_c
assert "xmm-cross-8" in vpxor_c and "ymm-cross-16" in vpxor_c

print("PASS WI-1794 static source, LASX preservation, gate and fixture contract")
PY
