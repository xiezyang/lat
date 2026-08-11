#!/usr/bin/env bash
set -euo pipefail
root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
python3 - "$root" <<'PY'
import sys
from pathlib import Path

root = Path(sys.argv[1])
avx = (root / "target/i386/latx/translator/tr-avx.c").read_text()
vpaes = (root / "target/i386/latx/translator/tr-vpaes.c").read_text()
header = (root / "target/i386/latx/include/translate.h").read_text()
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
cpu = (root / "target/i386/cpu.c").read_text()
minimal_asm = (root / "tests/integration/wi1916-fixtures/latx-avx-wi1916-cpuid-minimal.S").read_text()
minimal_c = (root / "tests/integration/wi1916-fixtures/latx-avx-wi1916-cpuid-minimal.c").read_text()
names = [
    "vaesdec", "vaesdeclast", "vaesenc", "vaesenclast",
    "vaesimc", "vaeskeygenassist", "vpclmulqdq",
]

for name in names:
    assert f"TRANS_FUNC_DEF({name}_lsx)" in header
    assert f"translate_register_lsx(dt_X86_INS_{name.upper()}," in dispatch

start = dispatch.index("void translate_context_init")
end = dispatch.index("\nbool ir1_translate", start)
context = dispatch[start:end]
registration = context[context.index("if (!option_enable_lasx) {"):]
for name in names:
    assert f"translate_{name}_lsx" in registration

def body(text, marker, next_marker):
    begin = text.index(marker)
    end = text.index(next_marker, begin)
    return text[begin:end]

vpclmul_lsx = body(avx, "bool translate_vpclmulqdq_lsx", "static void adjust_vsib_index")
assert "la_xv" not in vpclmul_lsx
assert "load_ymm_high128_shadow" in vpclmul_lsx
assert "store_ymm_high128_shadow" in vpclmul_lsx
assert "emit_pclmul_ctz_loop" in avx[avx.index("static void emit_pclmul_lsx_lane"):avx.index("bool translate_vpclmulqdq_lsx")]

vpaes_lsx = body(vpaes, "static bool translate_vaes_round_lsx", "bool latx_translate_aesenc_vpaes")
assert "la_xv" not in vpaes_lsx
assert "emit_aes_round_lsx" in vpaes_lsx
assert "load_ymm_high128_shadow" in vpaes_lsx
assert "store_ymm_high128_shadow" in vpaes_lsx

assert "la_xv" in body(avx, "bool translate_vpclmulqdq(IR1_INST", "static void emit_pclmul_lsx_lane")
assert "la_xv" in body(vpaes, "static void emit_aes_round_lasx", "static bool translate_aes_round")
assert "TRANS_FUNC_GEN(VPCLMULQDQ, vpclmulqdq)" in dispatch
assert "TRANS_FUNC_GEN(VAESDEC, vaesdec)" in dispatch
assert "CPUID_7_0_ECX_VAES" in cpu
assert "CPUID_7_0_ECX_VPCLMULQDQ" in cpu
assert "TCG_7_0_ECX_FEATURES" in cpu
assert "uint64_t feat_7_0_ecx_mask" in cpu
assert "uint64_t tcg_7_0_ecx_mask" in cpu
assert "builtin_x86_defs[i].features[FEAT_7_0_ECX] &= feat_7_0_ecx_mask" in cpu
assert "feature_word_info[FEAT_7_0_ECX].tcg_features &= tcg_7_0_ecx_mask" in cpu
assert "vaesdec xmm0, xmm0, xmm1" in minimal_asm
assert "latx_avx_wi1916_cpuid_minimal_run" in minimal_c
print("PASS WI-1916 LSX source audit: 7 entries, centralized LASX=0 registration, LASX preserved, no la_xv in LSX helpers")
PY
