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
manifest = json.loads((root / "tests/integration/latx-avx-opt-only-manifest.json").read_text())
entry = next(item for item in manifest["entries"] if item["mnemonic"] == "vmovapd")
assert entry["manual_template_required"] is True
assert entry["coverage_status"] == "needs_manual_template"
for suffix in ("S", "c"):
    assert (root / f"tests/integration/latx-avx-single-vmovapd.{suffix}").is_file()

lasx_start = source.index("static bool translate_vmovaps_lasx")
lsx_start = source.index("bool translate_vmovaps_lsx", lasx_start)
lsx_end = source.index("bool translate_vmovapd_lsx", lsx_start)
assert "la_xv" in source[lasx_start:lsx_start]
lsx = source[lsx_start:lsx_end]
assert "la_xv" not in lsx
for text in ("vmovaps_check_alignment", "gen_test_page_flag_force",
             "load_ymm_high128_shadow", "store_ymm_high128_shadow",
             "clear_ymm_high128_shadow"):
    assert text in lsx

apd_start = source.index("bool translate_vmovapd_lsx")
apd_end = source.index("bool translate_vlddqu_lsx", apd_start)
assert source[apd_start:apd_end].strip() == (
    "bool translate_vmovapd_lsx(IR1_INST *pir1)\n"
    "{\n"
    "    return translate_vmovaps_lsx(pir1);\n"
    "}"
)
context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
assert context.count("translate_register_lsx(dt_X86_INS_VMOVAPD,") == 1
assert "translate_vmovapd_lsx" in context
assert "TRANS_FUNC_GEN(VMOVAPD, vmovapd)" in dispatch
assert "VMOVDQA" not in context[context.index("translate_register_lsx(dt_X86_INS_VMOVAPD,"):]

print("PASS WI-1895 VMOVAPD LSX source, LASX preservation and fixture contract")
PY
