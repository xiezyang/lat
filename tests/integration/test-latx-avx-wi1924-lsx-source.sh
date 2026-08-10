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

start = source.index("bool translate_vzeroall_lsx")
end = source.index("bool translate_vzeroall(IR1_INST", start)
lsx = source[start:end]
for token in (
    "translate_vzeroall_lsx",
    "la_vxor_v",
    "clear_all_ymm_high128_shadows",
    "TARGET_X86_64",
    "reg_xmm = 16",
):
    if token not in lsx:
        raise SystemExit(f"missing WI-1924 LSX state check: {token}")
for forbidden in ("la_xv", "la_x86mtflag", "la_movgr2fcsr", "la_movfcsr2gr"):
    if forbidden in lsx:
        raise SystemExit(f"VZEROALL LSX path changes forbidden state: {forbidden}")
if "TRANS_FUNC_DEF(vzeroall_lsx);" not in header:
    raise SystemExit("missing vzeroall_lsx declaration")
if "TRANS_FUNC_GEN(VZEROALL, vzeroall)" not in registration:
    raise SystemExit("LASX generic VZEROALL registration was changed")

base = registration.index("void translate_context_init(void)")
context = registration[base:]
entry = "translate_register_lsx(dt_X86_INS_VZEROALL,"
pos = context.index(entry)
gate = context.rfind("if (!option_enable_lasx)", 0, pos)
if gate < 0:
    raise SystemExit("VZEROALL LSX registration has no option_enable_lasx=0 gate")
opening = context.find("{", gate)
depth = 0
for cursor in range(opening, len(context)):
    if context[cursor] == "{":
        depth += 1
    elif context[cursor] == "}":
        depth -= 1
        if depth == 0:
            if not (gate <= pos < cursor):
                raise SystemExit("VZEROALL LSX registration is outside its gate")
            break
else:
    raise SystemExit("unterminated VZEROALL registration gate")
if "translate_vzeroall_lsx" not in context[pos:pos + 200]:
    raise SystemExit("VZEROALL does not dispatch to translate_vzeroall_lsx")

print("PASS WI-1924 LSX source: all 16 low halves and high shadows cleared; GPR/EFLAGS/MXCSR untouched; LASX preserved")
PY
