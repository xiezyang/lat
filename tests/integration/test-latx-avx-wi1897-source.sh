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
for mnemonic in ("vmovupd", "vmovups"):
    entry = next(item for item in manifest["entries"] if item["mnemonic"] == mnemonic)
    assert entry["runner"] == "tests/integration/test-latx-avx-wi1897-fixtures.sh"
    assert entry["coverage_status"] in {"needs_manual_template", "existing_fixture"}
assert (root / "tests/integration/latx-avx-single-vmovupd.S").is_file()
assert (root / "tests/integration/latx-avx-single-vmovupd.c").is_file()
assert (root / "tests/integration/latx-avx-single-vmovups.S").is_file()
assert (root / "tests/integration/latx-avx-single-vmovups.c").is_file()

start = source.index("bool translate_vmovups_lsx")
end = source.index("bool translate_vmovups(IR1_INST", start)
lsx = source[start:end]
assert "la_xv" not in lsx
for text in ("load_v128_from_ir1_mem_exact", "store_v128_to_ir1_mem_exact",
             "load_v256_from_ir1_mem_exact", "store_v256_to_ir1_mem_exact",
             "load_ymm_high128_shadow", "store_ymm_high128_shadow",
             "clear_ymm_high128_shadow"):
    assert text in lsx
assert source.count("bool translate_vmovupd_lsx") == 1
assert source[source.index("bool translate_vmovupd_lsx"):source.index("bool translate_vmovups(IR1_INST")].count("return translate_vmovups_lsx") == 1
context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
assert context.count("translate_register_lsx(dt_X86_INS_VMOVUPD,") == 1
assert context.count("translate_register_lsx(dt_X86_INS_VMOVUPS,") == 1
assert "translate_vmovupd_lsx" in context and "translate_vmovups_lsx" in context
assert "TRANS_FUNC_GEN(VMOVUPD, vmovupd)" in dispatch
assert "TRANS_FUNC_GEN(VMOVUPS, vmovups)" in dispatch
print("PASS WI-1897 VMOVUPD/VMOVUPS LSX source, LASX preservation and fixture contract")
PY
