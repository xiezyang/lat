#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
source_file=$root/target/i386/latx/translator/tr-avx.c
output_dir=${LATX_AVX_WI1794_OUTPUT_DIR:-/tmp/latx-avx-wi1794-logic}

python3 - "$source_file" "$root/target/i386/latx/translator/translate.c" <<'PY'
import sys

source = open(sys.argv[1]).read()
dispatch = open(sys.argv[2]).read()
context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]

for name in ("pand", "por", "pxor"):
    marker = f"bool translate_v{name}_lsx"
    start = source.index(marker)
    brace = source.index("{", start)
    depth = 0
    end = None
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                end = pos + 1
                break
    assert end is not None, name
    body = source[start:end]
    assert f"bool translate_v{name}(IR1_INST * pir1)" in source, \
        f"{name}: LASX function missing"
    assert "la_xv" not in body, f"{name}: LSX body emits LASX"
    assert f"translate_v{name}(pir1)" not in body, f"{name}: LSX delegates to LASX"
    assert "option_enable_lasx" not in body, \
        f"{name}: backend selection is inside LSX function"

    opcode = "V" + name.upper()
    assert f"TRANS_FUNC_GEN({opcode}, v{name})" in dispatch, \
        f"{name}: LASX entry missing"
    registration = (
        f"translate_register_lsx(dt_X86_INS_{opcode}, "
        f"translate_v{name}_lsx);"
    )
    assert registration in context, f"{name}: LSX registration missing"

print("PASS WI-1794 LSX source/dispatch contract: vpand/vpor/vpxor")
PY

ninja -C "$root/build64" -j2

for spec in "vpand logic" "vpor logic" "vpxor reference"; do
    set -- $spec
    mnemonic=$1
    case_name=$2
    LATX_SSH_CONFIG=${LATX_SSH_CONFIG:-/home/xzy/.ssh/config} \
        python3 "$root/tests/integration/run-latx-avx-three-way.py" \
        --execute --mnemonic "$mnemonic" --case "$case_name" \
        --output-dir "$output_dir"
done

python3 - "$output_dir" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
for mnemonic in ("vpand", "vpor", "vpxor"):
    result = json.loads((root / f"{mnemonic}.json").read_text())
    assert result["status"] == "pass_x86_stdout_status", result
    assert result["x86_is_unique_baseline"] is True
    assert result["runs"]["lsx"]["option_enable_lasx_readback"] == 0
    assert result["runs"]["lsx"]["option_enable_lasx_readback_confirmed"] is True
print("PASS WI-1794 three-way logic cases: vpand/vpor/vpxor")
PY
