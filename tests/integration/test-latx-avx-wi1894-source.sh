#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

python3 - "$root" <<'PY'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
source = (root / "target/i386/latx/translator/tr-avx-mov.c").read_text()
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
manifest = json.loads(
    (root / "tests/integration/latx-avx-opt-only-manifest.json").read_text()
)

for mnemonic in ("vmaskmovpd", "vmaskmovps"):
    entry = next(item for item in manifest["entries"] if item["mnemonic"] == mnemonic)
    assert entry["manual_template_required"] is True
    assert entry["coverage_status"] == "needs_manual_template"
    assert (root / f"tests/integration/latx-avx-single-{mnemonic}.S").is_file()
    assert (root / f"tests/integration/latx-avx-single-{mnemonic}.c").is_file()

lsx_start = source.index("bool translate_vmaskmovpx_lsx")
lsx_end = source.index("bool translate_vmovq_lsx", lsx_start)
lsx = source[lsx_start:lsx_end]
assert "la_xv" not in lsx
assert "la_vslti_w" in lsx and "la_vslti_d" in lsx
for text in ("load_v128_from_ir1_mem_exact", "load_v256_from_ir1_mem_exact",
             "load_ymm_high128_shadow", "store_ymm_high128_shadow",
             "clear_ymm_high128_shadow", "store_v256_to_ir1_mem_exact"):
    assert text in lsx

context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
assert context.count("translate_register_lsx(dt_X86_INS_VMASKMOVPD,") == 1
assert context.count("translate_register_lsx(dt_X86_INS_VMASKMOVPS,") == 1
assert "translate_vmaskmovpx_lsx" in context
assert "TRANS_FUNC_GEN(VMASKMOVPD, vmaskmovpx)" in dispatch
assert "TRANS_FUNC_GEN(VMASKMOVPS, vmaskmovpx)" in dispatch
assert "VMASKMOVDQU" not in context

print("PASS WI-1894 LSX source, LASX preservation and fixture contract")
PY
