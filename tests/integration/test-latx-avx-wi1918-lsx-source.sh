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

start = source.index("static void vpmaskmov_lsx_lane")
end = source.index("\nbool translate_vpmaskmovx(", start)
lsx = source[start:end]
required = [
    "vpmaskmov_lsx_lane",
    "gen_test_page_flag_force",
    "gen_test_page_flag_force_range",
    "la_bstrpick_d",
    "la_beq",
    "PAGE_READ",
    "PAGE_WRITE | PAGE_WRITE_ORG",
    "clear_ymm_high128_shadow",
    "load_ymm_high128_shadow",
    "store_ymm_high128_shadow",
]
for token in required:
    if token not in lsx:
        raise SystemExit(f"missing LSX semantic check: {token}")
if "la_xv" in lsx:
    raise SystemExit("LSX VPMASKMOVD/Q path contains LASX instruction")
if "TRANS_FUNC_DEF(vpmaskmovx_lsx);" not in header:
    raise SystemExit("missing VPMASKMOVD/Q LSX declaration")
for opcode in ("VPMASKMOVD", "VPMASKMOVQ"):
    generic = f"TRANS_FUNC_GEN({opcode}, vpmaskmovx)"
    if generic not in registration:
        raise SystemExit(f"LASX generic {opcode} registration was changed")
    marker = f"dt_X86_INS_{opcode}"
    pos = registration.index(marker, registration.index("void translate_context_init"))
    gate = registration.rfind("if (!option_enable_lasx)", 0, pos)
    close = registration.rfind("}", 0, pos)
    if gate < close:
        raise SystemExit(f"{opcode} LSX registration is outside option_enable_lasx=0")
    if "translate_vpmaskmovx_lsx" not in registration[pos:pos + 180]:
        raise SystemExit(f"{opcode} does not dispatch to translate_vpmaskmovx_lsx")

print("PASS WI-1918 LSX source: mask sign-bit gating, per-lane forced faults, XMM/YMM shadow handling, LASX preserved")
PY
