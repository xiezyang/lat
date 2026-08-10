#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

python3 - "$root" <<'PY'
import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
cmp_source = (root / "target/i386/latx/translator/tr-avx-cmp.c").read_text()
header = (root / "target/i386/latx/include/translate.h").read_text()
translate = (root / "target/i386/latx/translator/translate.c").read_text()
manifest = json.loads(
    (root / "tests/integration/latx-avx-opt-only-manifest.json").read_text()
)

expected = {
    entry["mnemonic"]
    for entry in manifest["entries"]
    if entry["mnemonic"].startswith(("vpcmpeq", "vpcmpgt"))
}
assert expected == {
    "vpcmpeqb", "vpcmpeqw", "vpcmpeqd", "vpcmpeqq",
    "vpcmpgtb", "vpcmpgtw", "vpcmpgtd", "vpcmpgtq",
}

table_start = header.index("#define LATX_AVX_INTEGER_CMP_LSX_TABLE")
table_end = header.index("#define TRANS_FPU_WRAP_GEN_NO_PROLOGUE", table_start)
table = header[table_start:table_end]
rows = re.findall(
    r"X\((VPCMPEQ[BWQD]|VPCMPGT[BWQD]), "
    r"(pcmpeq[bwdq]|pcmpgt[bwdq]), (la_vs(?:eq|lt)_[bhwd]), (true|false)\)",
    table,
)
assert len(rows) == 8, rows
assert {opcode.lower() for opcode, _, _, _ in rows} == expected
assert {name for _, name, _, _ in rows} == {name[1:] for name in expected}
assert {reverse for _, _, _, reverse in rows} == {"true", "false"}

generated_start = cmp_source.index(
    "typedef IR2_INST *(*latx_avx_integer_cmp_lsx_fn)"
)
generated_end = cmp_source.index("bool translate_vpcmpeqx", generated_start)
generated = cmp_source[generated_start:generated_end]
assert "translate_avx_integer_cmp_lsx" in generated
assert "LATX_AVX_INTEGER_CMP_LSX_TABLE(LATX_AVX_INTEGER_CMP_LSX_DEFINE)" in generated
assert "translate_v##name##_lsx" in generated
assert "if (reverse)" in generated
assert "cmp(dest, rhs, lhs)" in generated
assert "load_v128_from_ir1_mem_exact" in generated
assert "load_v256_from_ir1_mem_exact" in generated
assert "load_ymm_high128_shadow" in generated
assert "store_ymm_high128_shadow" in generated
assert "clear_ymm_high128_shadow" in generated
assert "la_xv" not in generated
assert "option_enable_lasx" not in generated

lasx_start = cmp_source.index("bool translate_vpcmpeqx")
lasx_end = cmp_source.index("bool translate_vpcmpgtx", lasx_start)
lasx_cmp = cmp_source[lasx_start:lasx_end]
lasx_gt_start = lasx_end
lasx_gt_end = cmp_source.index("bool translate_vcmpeqpd", lasx_gt_start)
lasx_gt = cmp_source[lasx_gt_start:lasx_gt_end]
assert "la_xvseq_b" in lasx_cmp
assert "la_xvseq_d" in lasx_cmp
assert "la_xvslt_b" in lasx_gt
assert "la_xvslt_d" in lasx_gt

decl = header[header.index("#define LATX_AVX_INTEGER_CMP_LSX_DECL"):]
assert "LATX_AVX_INTEGER_CMP_LSX_TABLE(LATX_AVX_INTEGER_CMP_LSX_DECL)" in decl
assert "TRANS_FUNC_DEF(v##name##_lsx);" in decl

register_start = translate.index("#define LATX_AVX_INTEGER_CMP_LSX_REGISTER")
register_end = translate.index("#define LATX_AVX_INTEGER_3OP_LSX_REGISTER", register_start)
register = translate[register_start:register_end]
assert "LATX_AVX_INTEGER_CMP_LSX_TABLE(\n            LATX_AVX_INTEGER_CMP_LSX_REGISTER)" in register
assert "translate_register_lsx(dt_X86_INS_##opcode" in register
assert "translate_v##name##_lsx" in register
assert "translate_register_lsx(dt_X86_INS_VPCMPEQQ" not in translate

print("PASS WI-1840 compare X-macro, LSX helper and LASX preservation contract")
print("PASS WI-1840 header declarations and centralized registration contract")
PY
