#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
python3 - "$root" <<'PY'
import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
source = (root / "target/i386/latx/translator/tr-avx-mov.c").read_text()
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
manifest = json.loads((root / "tests/integration/latx-avx-opt-only-manifest.json").read_text())

for mnemonic in ("vmovhlps", "vmovlhps"):
    entry = next(item for item in manifest["entries"] if item["mnemonic"] == mnemonic)
    assert entry["runner"] == "tests/integration/test-latx-avx-wi1903-fixtures.sh"
    assert entry["coverage_status"] == "existing_fixture"
    assert len(entry["source_files"]) == 2
    for file_name in entry["source_files"]:
        assert (root / file_name).is_file()

def body(name, next_name):
    start = source.index("bool translate_" + name + "(")
    end = source.index("bool translate_" + next_name + "(", start)
    return source[start:end]

hlps = body("vmovhlps", "vmovhpd")
lhps_start = source.index("bool translate_vmovlhps(")
lhps_end = source.index("bool translate_vmovntdqa(", lhps_start)
lhps = source[lhps_start:lhps_end]
for text in (hlps, lhps):
    assert text.count("ir1_opnd_is_xmm") == 3
    assert "load_freg128_from_ir1" in text
    assert "set_high128_xreg_to_zero" in text
    assert "la_xv" not in text
assert "la_vilvh_d(dest, src1, src2)" in hlps
assert "la_vpickev_d(dest, src2, src1)" in lhps

lsx_start = source.index("static void emit_vmovhlps_lsx_lane(")
lsx_end = source.index("bool translate_vmovhlps(", lsx_start)
lsx = source[lsx_start:lsx_end]
for name in ("translate_vmovhlps_lsx", "translate_vmovlhps_lsx"):
    assert "bool " + name + "(" in lsx
assert "load_ymm_high128_shadow" in lsx
assert "store_ymm_high128_shadow" in lsx
assert "clear_ymm_high128_shadow" in lsx
assert "ir1_opnd_is_ymm" in lsx
assert "ir1_opnd_is_mem" not in lsx
assert "set_high128_xreg_to_zero" not in lsx
assert "la_xv" not in lsx
assert "emit_vmovhlps_lsx_lane(low_result" in lsx
assert "emit_vmovhlps_lsx_lane(high_result" in lsx

context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
assert "TRANS_FUNC_GEN(VMOVHLPS, vmovhlps)" in dispatch
assert "TRANS_FUNC_GEN(VMOVLHPS, vmovlhps)" in dispatch
assert "translate_register_lsx(dt_X86_INS_VMOVHLPS" in context
assert "translate_register_lsx(dt_X86_INS_VMOVLHPS" in context
assert re.search(r"TRANS_FUNC_GEN\(VMOVHLPS, vmovhlps\)", dispatch)
assert re.search(r"TRANS_FUNC_GEN\(VMOVLHPS, vmovlhps\)", dispatch)

for mnemonic in ("vmovhlps", "vmovlhps"):
    assembly = (root / f"tests/integration/latx-avx-single-{mnemonic}.S").read_text()
    assert assembly.count(f"{mnemonic} xmm") == 4
    assert not re.search(rf"{mnemonic} xmm[^\n]*\[", assembly)
    c_source = (root / f"tests/integration/latx-avx-single-{mnemonic}.c").read_text()
    assert "0x7ff8000000000042" in c_source
    assert "0x7ff0000000000001" in c_source
    assert "0x8000000000000000" in c_source
    assert "source_a_nan" in c_source

print("PASS WI-1903 source audit: LASX preserved, LSX lane paths registered, fixtures register-only")
PY
