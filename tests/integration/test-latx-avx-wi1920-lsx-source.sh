#!/usr/bin/env bash
set -euo pipefail

repo=${1:-.}
python3 - "$repo" <<'PY'
import re
import sys
from pathlib import Path

repo = Path(sys.argv[1])
source = (repo / "target/i386/latx/translator/tr-avx.c").read_text()
header = (repo / "target/i386/latx/include/translate.h").read_text()
registration = (repo / "target/i386/latx/translator/translate.c").read_text()

start = source.index("static bool translate_vunpckxx_lsx")
end = source.index("static uint8_t map_vshufpd_lsx_imm", start)
lsx = source[start:end]
required = [
    "translate_vunpckhpd_lsx",
    "translate_vunpckhps_lsx",
    "translate_vunpcklpd_lsx",
    "translate_vunpcklps_lsx",
    "load_avx_lsx_operand",
    "store_avx_lsx_result",
    "tr_save_ymm_to_env(UINT16_MAX)",
    "la_vilvh_d",
    "la_vilvl_d",
    "la_vilvh_w",
    "la_vilvl_w",
    "ir1_opnd_is_ymm",
]
for token in required:
    if token not in lsx:
        raise SystemExit(f"missing LSX unpack semantic check: {token}")
if "la_xv" in lsx:
    raise SystemExit("WI-1920 LSX unpack path contains LASX instruction")
if "clear_ymm_high128_shadow" not in source:
    raise SystemExit("XMM destination high-half clearing helper is missing")

for name in ("vunpckhpd", "vunpckhps", "vunpcklpd", "vunpcklps"):
    if f"TRANS_FUNC_DEF({name}_lsx);" not in header:
        raise SystemExit(f"missing declaration: {name}_lsx")

base = registration.index("void translate_context_init(void)")
context = registration[base:]

def enclosing_gate(pos):
    gate = context.rfind("if (!option_enable_lasx)", 0, pos)
    if gate < 0:
        return False
    opening = context.find("{", gate)
    depth = 0
    for cursor in range(opening, len(context)):
        if context[cursor] == "{":
            depth += 1
        elif context[cursor] == "}":
            depth -= 1
            if depth == 0:
                return gate <= pos < cursor
    return False

for opcode, function in (
    ("VUNPCKHPD", "translate_vunpckhpd_lsx"),
    ("VUNPCKHPS", "translate_vunpckhps_lsx"),
    ("VUNPCKLPD", "translate_vunpcklpd_lsx"),
    ("VUNPCKLPS", "translate_vunpcklps_lsx"),
):
    generic = f"TRANS_FUNC_GEN({opcode}, {opcode.lower()})"
    if generic not in registration:
        raise SystemExit(f"LASX generic {opcode} registration was changed")
    entry = f"translate_register_lsx(dt_X86_INS_{opcode},"
    pos = context.index(entry)
    if not enclosing_gate(pos):
        raise SystemExit(f"{opcode} LSX registration is outside option_enable_lasx=0")
    if function not in context[pos:pos + 120]:
        raise SystemExit(f"{opcode} does not dispatch to {function}")

print("PASS WI-1920 LSX source: four half-local VUNPCK paths, XMM/YMM shadow handling, page-checked operands, LASX preserved")
PY
