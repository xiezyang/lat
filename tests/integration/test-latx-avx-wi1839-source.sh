#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

python3 - "$root" <<'PY'
import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
header = (root / "target/i386/latx/include/translate.h").read_text()
source = (root / "target/i386/latx/translator/tr-avx.c").read_text()
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
manifest = json.loads(
    (root / "tests/integration/latx-avx-opt-only-manifest.json").read_text()
)

manifest_names = {
    entry["mnemonic"]
    for entry in manifest["entries"]
    if entry["mnemonic"].startswith(("vpadd", "vpsub", "vpmin", "vpmax"))
}
assert len(manifest_names) == 28, sorted(manifest_names)

table_start = header.index("#define LATX_AVX_INTEGER_3OP_LSX_TABLE(X)")
table_end = header.index("#define LATX_AVX_INTEGER_REMAINING_3OP_LSX_TABLE(X)",
                         table_start)
table = header[table_start:table_end]
rows = re.findall(r"X\((V[A-Z0-9]+), ([a-z0-9]+), (la_v[a-z0-9_]+)\)", table)
assert len(rows) == 28, rows
table_names = {opcode.lower() for opcode, _, _ in rows}
assert table_names == manifest_names, sorted(table_names ^ manifest_names)
assert len({name for _, name, _ in rows}) == 28

for opcode, name, lsx_op in rows:
    assert f"{opcode}, {name}, {lsx_op}" in table

assert "typedef IR2_INST *(*latx_avx_integer_3op_lsx_fn)" in source
helper_start = source.index("static bool translate_avx_integer_3op_lsx")
helper_end = source.index("#define LATX_AVX_INTEGER_3OP_LSX_DEFINE", helper_start)
helper = source[helper_start:helper_end]
assert "if (ir1_opnd_is_ymm(opnd0))" in helper
assert "tr_save_ymm_to_env(UINT16_MAX)" in helper
define_start = source.index(
    "#define LATX_AVX_INTEGER_3OP_LSX_DEFINE"
)
define_end = source.index("bool translate_vpaddx", define_start)
generated = source[define_start:define_end]
assert "translate_v##name##_lsx" in generated
assert "translate_avx_integer_3op_lsx(pir1, lsx_op)" in generated
assert "la_xv" not in generated
assert "option_enable_lasx" not in generated

context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
registration = "LATX_AVX_INTEGER_3OP_LSX_TABLE(\n            LATX_AVX_INTEGER_3OP_LSX_REGISTER)"
assert context.count(registration) == 1
assert "translate_register_lsx(dt_X86_INS_##opcode" in context
for opcode, _, _ in rows:
    assert f"dt_X86_INS_{opcode}" not in context.replace(
        "dt_X86_INS_##opcode", ""
    )

for function in {entry["translator_functions"][0] for entry in manifest["entries"]
                 if entry["mnemonic"] in manifest_names}:
    assert f"bool translate_{function}" in source, function

print("PASS WI-1839 X-macro, LSX wrapper and centralized registration contract")
PY
