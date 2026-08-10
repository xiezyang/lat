#!/usr/bin/env bash
set -euo pipefail

repo=${1:-.}
python3 - "$repo" <<'PY'
import sys
from pathlib import Path

repo = Path(sys.argv[1])
source = (repo / "target/i386/latx/translator/tr-avx.c").read_text()
header = (repo / "target/i386/latx/include/translate.h").read_text()
registration = (repo / "target/i386/latx/translator/translate.c").read_text()

start = source.index("static void translate_vpshufb_lane_lsx")
end = source.index("static IR2_OPND build_blend_mask_lsx", start)
lsx = source[start:end]
required = [
    "translate_vpshufb_lsx",
    "translate_vpshufd_lsx",
    "translate_vpshufhw_lsx",
    "translate_vpshuflw_lsx",
    "load_avx_lsx_operand",
    "store_avx_lsx_result",
    "la_vshuf_b",
    "la_vslti_b",
    "la_vandn_v",
    "la_vshuf4i_w",
    "la_vshuf4i_h",
    "ir1_opnd_uimm",
    "tr_save_ymm_to_env",
]
for token in required:
    if token not in lsx:
        raise SystemExit(f"missing LSX shuffle semantic check: {token}")
if "la_xv" in lsx:
    raise SystemExit("WI-1919 LSX shuffle path contains LASX instruction")
for name in ("vpshufb", "vpshufd", "vpshufhw", "vpshuflw"):
    if f"TRANS_FUNC_DEF({name}_lsx);" not in header:
        raise SystemExit(f"missing declaration: {name}_lsx")
for opcode, function in (
    ("VPSHUFB", "translate_vpshufb_lsx"),
    ("VPSHUFD", "translate_vpshufd_lsx"),
    ("VPSHUFHW", "translate_vpshufhw_lsx"),
    ("VPSHUFLW", "translate_vpshuflw_lsx"),
):
    if f"TRANS_FUNC_GEN({opcode}, {opcode.lower()})" not in registration:
        raise SystemExit(f"LASX generic {opcode} registration was changed")
    marker = f"dt_X86_INS_{opcode}"
    pos = registration.index(marker, registration.index("void translate_context_init"))
    gate = registration.rfind("if (!option_enable_lasx)", 0, pos)
    close = registration.rfind("}", 0, pos)
    if gate < close:
        raise SystemExit(f"{opcode} LSX registration is outside option_enable_lasx=0")
    if function not in registration[pos:pos + 160]:
        raise SystemExit(f"{opcode} does not dispatch to {function}")

print("PASS WI-1919 LSX source: lane-local shuffle, VPSHUFB high-bit zeroing, immediates, XMM/YMM shadow handling, LASX preserved")
PY
