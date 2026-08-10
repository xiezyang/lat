#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

python3 - "$root" <<'PY'
import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
header = (root / "target/i386/latx/include/translate.h").read_text()
source = (root / "target/i386/latx/translator/tr-avx-shift.c").read_text()
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
manifest = json.loads(
    (root / "tests/integration/latx-avx-opt-only-manifest.json").read_text()
)

names = {
    entry["mnemonic"]
    for entry in manifest["entries"]
    if entry["mnemonic"].startswith(("vpsll", "vpsrl", "vpsra"))
}
assert len(names) == 15, sorted(names)

table_start = header.index("#define LATX_AVX_INTEGER_SHIFT_LSX_TABLE(X)")
table_end = header.index("#define TRANS_FPU_WRAP_GEN_NO_PROLOGUE", table_start)
rows = re.findall(
    r"X\((V[A-Z0-9]+), ([a-z0-9_]+)\)", header[table_start:table_end]
)
assert {opcode.lower() for opcode, _ in rows} == names
assert len(rows) == 15

lsx_start = source.index("typedef IR2_INST *(*latx_avx_shift_imm_fn)")
lsx_end = source.index("static bool translate_vpsrlq_lasx", lsx_start)
lsx = source[lsx_start:lsx_end]
assert "la_xv" not in lsx
assert "option_enable_lasx" not in lsx
assert "load_v128_from_ir1_mem_exact" in lsx
assert "load_v256_from_ir1_mem_exact" in lsx
assert "load_ymm_high128_shadow" in lsx
assert "store_ymm_high128_shadow" in lsx
assert "clear_ymm_high128_shadow" in lsx

context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
registration = "LATX_AVX_INTEGER_SHIFT_LSX_TABLE(\n            LATX_AVX_INTEGER_SHIFT_LSX_REGISTER)"
assert context.count(registration) == 1
assert "translate_register_lsx(dt_X86_INS_##opcode" in context

for mnemonic in sorted(names):
    assert (root / f"tests/integration/latx-avx-single-{mnemonic}.S").is_file()
    assert (root / f"tests/integration/latx-avx-single-{mnemonic}.c").is_file()

print("PASS WI-1842 15 mnemonic LSX source, registration and fixture contract")
PY
