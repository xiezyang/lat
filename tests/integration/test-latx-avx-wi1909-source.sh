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
for mnemonic in ("vmovshdup", "vmovsldup"):
    entry = next(item for item in manifest["entries"] if item["mnemonic"] == mnemonic)
    assert entry["runner"] == "tests/integration/test-latx-avx-wi1909-fixtures.sh"
    assert entry["coverage_status"] == "existing_fixture"
    for file_name in entry["source_files"]:
        assert (root / file_name).is_file()

def function(name, next_name):
    start = source.index("bool translate_" + name + "(")
    end = source.index("bool translate_" + next_name + "(", start)
    return source[start:end]

lsx_start = source.index("static bool translate_vmovsdup_lsx(")
lsx_end = source.index("bool translate_vmovshdup_lsx(", lsx_start)
lsx = source[lsx_start:lsx_end]
assert "la_xv" not in lsx
assert "la_vpackod_w" in lsx
assert "la_vpackev_w" in lsx
assert "load_v128_from_ir1_mem_exact" in lsx
assert "load_v256_from_ir1_mem_exact" in lsx
assert "load_ymm_high128_shadow" in lsx
assert "store_ymm_high128_shadow" in lsx
assert "clear_ymm_high128_shadow" in lsx
assert lsx.count("la_vpackod_w") == 3
assert lsx.count("la_vpackev_w") == 3

shdup = function("vmovshdup", "vmovsldup")
sldup = function("vmovsldup", "vmovddup_lsx")
assert "la_xvpackod_w" in shdup
assert "la_xvpackev_w" in sldup

context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
assert "TRANS_FUNC_GEN(VMOVSHDUP, vmovshdup)" in dispatch
assert "TRANS_FUNC_GEN(VMOVSLDUP, vmovsldup)" in dispatch
assert "dt_X86_INS_VMOVSHDUP" in context
assert "dt_X86_INS_VMOVSLDUP" in context
assert "translate_vmovshdup_lsx" in source + dispatch
assert "translate_vmovsldup_lsx" in source + dispatch
declarations = (root / "target/i386/latx/include/translate.h").read_text()
assert "TRANS_FUNC_DEF(vmovshdup_lsx)" in declarations
assert "TRANS_FUNC_DEF(vmovsldup_lsx)" in declarations

for mnemonic in ("vmovshdup", "vmovsldup"):
    assembly = (root / f"tests/integration/latx-avx-single-{mnemonic}.S").read_text()
    assert assembly.count(f"{mnemonic} ") == 8
    assert re.search(rf"{mnemonic} xmm0, xmm1", assembly)
    assert re.search(rf"{mnemonic} ymm0, ymm1", assembly)
    assert re.search(rf"{mnemonic} xmm0, xmmword ptr", assembly)
    assert re.search(rf"{mnemonic} ymm0, ymmword ptr", assembly)
    c_source = (root / f"tests/integration/latx-avx-single-{mnemonic}.c").read_text()
    assert "0x7ff8000000000042" in c_source
    assert "0x7ff0000000000001" in c_source
    assert "SYS_MPROTECT" in c_source

print("PASS WI-1909 source audit: LASX preserved, LSX helper has no la_xv*, exact XMM/YMM memory paths and centralized registrations present")
PY
