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

for mnemonic in ("vmovhps", "vmovlps"):
    entry = next(item for item in manifest["entries"] if item["mnemonic"] == mnemonic)
    assert entry["runner"] == "tests/integration/test-latx-avx-wi1905-fixtures.sh"
    assert entry["coverage_status"] == "existing_fixture"
    assert len(entry["source_files"]) == 2
    for file_name in entry["source_files"]:
        assert (root / file_name).is_file()

def body(name, next_name):
    start = source.index("bool translate_" + name + "(")
    end = source.index("bool translate_" + next_name + "(", start)
    return source[start:end]

hps = body("vmovhps", "vmovlhps")
lps = body("vmovlps", "vmovhlps")
hps_lsx = body("vmovhps_lsx", "vmovlhps")
lps_lsx = body("vmovlps_lsx", "vmovhlps")
assert "translate_vmovhpd(pir1)" in hps
assert "translate_vmovlpd(pir1)" in lps
assert "translate_vmovhpd_lpd_lsx(pir1, true)" in hps_lsx
assert "translate_vmovhpd_lpd_lsx(pir1, false)" in lps_lsx
hpd = body("vmovhpd", "vmovhps")
lpd = body("vmovlpd", "vmovhlps")
assert "la_xv" in lpd
assert "la_xv" not in hpd
assert "set_high128_xreg_to_zero" in hpd
assert "set_high128_xreg_to_zero" in lpd

context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
assert "TRANS_FUNC_GEN(VMOVHPS, vmovhps)" in dispatch
assert "TRANS_FUNC_GEN(VMOVLPS, vmovlps)" in dispatch
assert "translate_register_lsx(dt_X86_INS_VMOVHPS,\n                                   translate_vmovhps_lsx)" in context
assert "translate_register_lsx(dt_X86_INS_VMOVLPS,\n                                   translate_vmovlps_lsx)" in context

for mnemonic in ("vmovhps", "vmovlps"):
    assembly = (root / f"tests/integration/latx-avx-single-{mnemonic}.S").read_text()
    assert assembly.count(f"{mnemonic} ") == 6
    assert f"{mnemonic} xmm0, xmm1, qword ptr" in assembly
    assert f"{mnemonic} qword ptr" in assembly
    c_source = (root / f"tests/integration/latx-avx-single-{mnemonic}.c").read_text()
    assert "0x7ff8000000000042" in c_source
    assert "0x7ff0000000000001" in c_source
    assert "LATX_SYS_MPROTECT" in c_source

print("PASS WI-1905 source audit: no overlap, LASX preserved, LSX gap recorded, load/store/fault fixtures present")
PY
