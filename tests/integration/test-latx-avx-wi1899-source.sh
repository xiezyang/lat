#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
python3 - "$root" <<'PY'
import json
import sys
from pathlib import Path
root = Path(sys.argv[1])
source = (root / "target/i386/latx/translator/tr-avx-mov.c").read_text()
simd_mov = (root / "target/i386/latx/translator/tr-simd-mov.c").read_text()
opnd_process = (root / "target/i386/latx/translator/tr-opnd-process.c").read_text()
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
manifest = json.loads((root / "tests/integration/latx-avx-opt-only-manifest.json").read_text())
entry = next(item for item in manifest["entries"] if item["mnemonic"] == "vmaskmovdqu")
assert entry["manual_template_required"] is True
assert entry["coverage_status"] == "needs_manual_template"
assert (root / "tests/integration/latx-avx-single-vmaskmovdqu.S").is_file()
assert (root / "tests/integration/latx-avx-single-vmaskmovdqu.c").is_file()
lsx_start = simd_mov.index("bool translate_maskmovdqu_lsx")
lsx_end = simd_mov.index("bool translate_movupd", lsx_start)
lsx = simd_mov[lsx_start:lsx_end]
assert "la_xv" not in lsx
for text in ("edi_index", "load_v128_from_guest_addr_exact",
             "store_v128_to_guest_addr_exact", "la_vandi_b", "la_vseq_b",
             "la_vnor_v"):
    assert text in lsx
assert "bool translate_maskmovdqu(IR1_INST" in simd_mov
legacy_start = simd_mov.index("bool translate_maskmovdqu(IR1_INST")
legacy_end = simd_mov.index("bool translate_maskmovdqu_lsx", legacy_start)
assert "la_vld" in simd_mov[legacy_start:legacy_end]
assert "la_vst" in simd_mov[legacy_start:legacy_end]
for helper in ("load_v128_from_guest_addr_exact", "store_v128_to_guest_addr_exact"):
    helper_start = opnd_process.index(helper)
    helper_end = opnd_process.index("\n}", helper_start)
    assert "la_xv" not in opnd_process[helper_start:helper_end]
context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
registration = context[context.index("if (!option_enable_lasx || !option_avx_cpuid)"):]
assert "if (!option_enable_lasx)" in registration
assert "translate_register_lsx(dt_X86_INS_VMASKMOVDQU," in registration
assert "translate_maskmovdqu_lsx" in registration
assert "TRANS_FUNC_GEN(VMASKMOVDQU, maskmovdqu)" in dispatch
assert "TRANS_FUNC_GEN(VMASKMOVPD, vmaskmovpx)" in dispatch
assert "TRANS_FUNC_GEN(VMASKMOVPS, vmaskmovpx)" in dispatch
print("PASS WI-1899 VMASKMOVDQU LSX implementation, WI-1894 boundary and fixture contract")
PY
