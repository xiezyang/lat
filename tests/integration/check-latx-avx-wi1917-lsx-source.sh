#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
python3 - "$root" <<'PY'
from pathlib import Path
import sys

root = Path(sys.argv[1])
names = "vpgatherdd vpgatherdq vpgatherqd vpgatherqq vgatherdps vgatherdpd vgatherqps vgatherqpd".split()
header = (root / "target/i386/latx/include/translate.h").read_text()
source = (root / "target/i386/latx/translator/tr-avx.c").read_text()
registration = (root / "target/i386/latx/translator/translate.c").read_text()

for name in names:
    function = f"translate_{name}_lsx"
    assert f"bool {function}" in source, name
    assert f"TRANS_FUNC_DEF({name}_lsx)" in header, name
    assert f"dt_X86_INS_{name.upper()}" in registration, name
    assert function in registration, name

assert "TRANS_FUNC_GEN(VPGATHERDD, vpgatherdd)" in registration
assert "TRANS_FUNC_GEN(VGATHERDPD, vpgatherdq)" in registration
start = source.rindex("static void translate_avx_gather_lane_lsx")
block = source[start:]
for token in (
    "la_blt(mask_value, zero_ir2_opnd, load)",
    "tr_save_ymm_to_env(UINT16_MAX)",
    "ir1_opnd_vsib_index_reg_num",
    "ir1_index_reg_is_ymm(opnd1)",
    "index_high_values",
    "mask_high_values",
    "index_lane += lanes_per_half",
    "adjust_vsib_index(address, base_addr, index_value",
    "la_ld_w(loaded, address, 0)",
    "la_ld_d(loaded, address, 0)",
    "store_ymm_high128_shadow",
):
    assert token in block, token
assert "la_xv" not in block
print("PASS WI-1917 LSX source: 8 functions/declarations/dispatches, masked VSIB loads, high-half state")
PY
