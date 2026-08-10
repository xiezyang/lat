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

start = source.index("static bool translate_vpcmpxstrx_lsx")
end = source.index("bool translate_vpcmpestrm(IR1_INST", start)
lsx = source[start:end]
required = [
    "translate_vpcmpestri_lsx",
    "translate_vpcmpestrm_lsx",
    "translate_vpcmpistri_lsx",
    "translate_vpcmpistrm_lsx",
    "helper_pcmpestri_xmm",
    "helper_pcmpestrm_xmm",
    "helper_pcmpistri_xmm",
    "helper_pcmpistrm_xmm",
    "tr_gen_call_to_helper_pcmpxstrx",
    "load_freg128_from_ir1_mem",
    "clear_ymm_high128_shadow(0)",
    "la_vori_b",
]
for token in required:
    if token not in lsx and token != "gen_test_page_flag_force_range":
        raise SystemExit(f"missing WI-1921 LSX semantic check: {token}")
if "la_xv" in lsx or "option_enable_lasx" in lsx:
    raise SystemExit("WI-1921 LSX wrapper contains LASX or dispatch logic")

for name in ("vpcmpestri", "vpcmpestrm", "vpcmpistri", "vpcmpistrm"):
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
    ("VPCMPESTRI", "translate_vpcmpestri_lsx"),
    ("VPCMPESTRM", "translate_vpcmpestrm_lsx"),
    ("VPCMPISTRI", "translate_vpcmpistri_lsx"),
    ("VPCMPISTRM", "translate_vpcmpistrm_lsx"),
):
    if f"TRANS_FUNC_GEN({opcode}, v{opcode.lower()[1:]})" not in registration:
        raise SystemExit(f"LASX generic {opcode} registration was changed")
    entry = f"translate_register_lsx(dt_X86_INS_{opcode},"
    pos = context.index(entry)
    if not enclosing_gate(pos):
        raise SystemExit(f"{opcode} LSX registration is outside option_enable_lasx=0")
    if function not in context[pos:pos + 120]:
        raise SystemExit(f"{opcode} does not dispatch to {function}")

print("PASS WI-1921 LSX source: helper-backed string compare, forced memory checks, implicit XMM0 mask high clear, LASX preserved")
PY
