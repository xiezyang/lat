#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
python3 - "$root" <<'PY'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
mnemonics = {
    "vpackssdw", "vpacksswb", "vpackusdw", "vpackuswb",
    "vpunpckhbw", "vpunpckhdq", "vpunpckhqdq", "vpunpckhwd",
    "vpunpcklbw", "vpunpckldq", "vpunpcklqdq", "vpunpcklwd",
    "vshufpd", "vshufps",
}
manifest = json.loads((root / "tests/integration/latx-avx-opt-only-manifest.json").read_text())
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
translator = (root / "target/i386/latx/translator/tr-avx.c").read_text()

for mnemonic in mnemonics:
    entry = next(item for item in manifest["entries"] if item["mnemonic"] == mnemonic)
    assert entry["coverage_status"] == "existing_fixture"
    if mnemonic == "vpunpcklqdq":
        assert entry["runner"] == "tests/integration/test-latx-avx-single-vpunpcklqdq.sh"
    else:
        assert entry["runner"] == "tests/integration/test-latx-avx-wi1843-fixtures.sh"
    assert len(entry["source_files"]) == 2
    for name in entry["source_files"]:
        assert (root / name).is_file()
    assembly = (root / f"tests/integration/latx-avx-single-{mnemonic}.S").read_text()
    if mnemonic == "vpunpcklqdq":
        assert assembly.count("vpunpcklqdq ") >= 12
        assert "latx_avx_single_vpunpcklqdq_fault_xmm" in assembly
        assert "latx_avx_single_vpunpcklqdq_fault_ymm" in assembly
        assert "latx_avx_single_vpunpcklqdq_observe_marker" in assembly
        continue
    assert assembly.count(f"    {mnemonic} ") == 12
    assert "xmm0" in assembly and "ymm0" in assembly
    assert "xmmword ptr [rip + input_b]" in assembly
    assert "ymmword ptr [rip + input_b]" in assembly
    if mnemonic in {"vshufpd", "vshufps"}:
        for imm in ("0", "85", "170", "255"):
            assert f", {imm}" in assembly
    else:
        assert "xmm0, xmm0, xmm0" in assembly
        assert "ymm0, ymm0, ymm0" in assembly
    assert "0x80017fff00010000" in assembly
    assert "0x7ff8000000000042" in assembly
    assert f"TRANS_FUNC_GEN({mnemonic.upper()}" in dispatch

assert "TRANS_FUNC_GEN(VMOVDDUP, vmovddup)" in dispatch
assert "TRANS_FUNC_GEN(VMOVNTDQA, vmovntdqa)" in dispatch
assert "TRANS_FUNC_GEN(VMOVSHDUP, vmovshdup)" in dispatch
assert "TRANS_FUNC_GEN(VMOVSLDUP, vmovsldup)" in dispatch
assert "la_xv" in translator
vshufps_lsx = translator.split(
    "static void translate_vshufps_lane_lsx", 1)[1].split(
    "bool translate_vshufps_lsx", 1)[0]
assert "la_vpickev_d(result, src2_shuffled, src1_shuffled)" in vshufps_lsx
assert "la_vpickod_d" not in vshufps_lsx
print("PASS WI-1843 source audit: 14 mnemonic fixture registration, legal reg/mem forms, alias/boundary coverage, LASX preserved")
PY
