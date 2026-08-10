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
header = (root / "target/i386/latx/include/translate.h").read_text()
manifest = json.loads((root / "tests/integration/latx-avx-opt-only-manifest.json").read_text())

for mnemonic, suffix in (("vmovhpd", "hpd"), ("vmovlpd", "lpd")):
    entry = next(item for item in manifest["entries"] if item["mnemonic"] == mnemonic)
    assert entry["runner"] == "tests/integration/test-latx-avx-wi1904-fixtures.sh"
    assert entry["coverage_status"] == "existing_fixture"
    assert entry["source_files"] == [
        f"tests/integration/latx-avx-single-{mnemonic}.S",
        f"tests/integration/latx-avx-single-{mnemonic}.c",
    ]
    assert (root / entry["source_files"][0]).is_file()
    assert (root / entry["source_files"][1]).is_file()
    assert f"TRANS_FUNC_GEN({mnemonic.upper()}, {mnemonic})" in dispatch
    assert f"bool translate_vmov{suffix}_lsx(IR1_INST *pir1)" in source
    assert f"translate_register_lsx(dt_X86_INS_{mnemonic.upper()}," in dispatch
    assert f"TRANS_FUNC_DEF(vmov{suffix}_lsx)" in header

assert "bool translate_vmovhpd(IR1_INST * pir1)" in source
assert "bool translate_vmovlpd(IR1_INST * pir1)" in source
lsx_start = source.index("static bool translate_vmovhpd_lpd_lsx(")
lsx_end = source.index("bool translate_vmovlpd(IR1_INST * pir1)", lsx_start)
lsx = source[lsx_start:lsx_end]
assert "load_u64_from_ir1_mem_exact" in lsx
assert "store_u64_to_ir1_mem_exact" in lsx
assert "clear_ymm_high128_shadow" in lsx
assert "la_xv" not in lsx
assert "load_freg128_from_ir1(src2)" in source
assert "store_ireg_to_ir1(temp, dest, false)" in source
for mnemonic in ("vmovhpd", "vmovlpd"):
    asm = (root / f"tests/integration/latx-avx-single-{mnemonic}.S").read_text()
    c = (root / f"tests/integration/latx-avx-single-{mnemonic}.c").read_text()
    for required in (
        f"{mnemonic} xmm0, xmm1, qword ptr [rsi]",
        f"{mnemonic} xmm0, xmm0, qword ptr [rsi + 8]",
        f"{mnemonic} qword ptr [rsi + 8], xmm0",
        f"{mnemonic} xmm15, xmm1, qword ptr [rdi]",
        f"{mnemonic} qword ptr [rdi], xmm15",
        "ymmword ptr",
    ):
        assert required in asm, (mnemonic, required)
    for required in ("fault-load", "fault-store", "WI1904_PAGE_SIZE",
                     "WI1904_RECORD_SIZE = 48", "WI1904_SYS_MPROTECT"):
        assert required in c, (mnemonic, required)

print("PASS WI-1904 source audit, LSX dispatch and fixture contract")
PY
