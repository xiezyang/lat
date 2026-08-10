#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
source_file=$root/target/i386/latx/translator/tr-avx-mov.c
output_dir=${LATX_AVX_WI1838_OUTPUT_DIR:-/tmp/latx-avx-wi1838-results-fixed}

python3 - "$source_file" <<'PY'
import sys

text = open(sys.argv[1]).read()
start = text.index("static bool translate_vmovaps_lasx")
end = text.index("bool translate_vmovaps_lsx", start)
body = text[start:end]
assert body.count("vmovaps_check_alignment(pir1, src, 32)") == 1
assert body.count("vmovaps_check_alignment(pir1, dest, 32)") == 1
assert body.count("vmovaps_check_alignment(pir1, src, 16)") == 1
assert body.count("vmovaps_check_alignment(pir1, dest, 16)") == 1
print("PASS WI-1838 LASX vmovaps alignment source contract")
PY

ninja -C "$root/build64" -j2

LATX_SSH_CONFIG=${LATX_SSH_CONFIG:-/home/xzy/.ssh/config} \
    python3 "$root/tests/integration/run-latx-avx-three-way.py" \
    --execute --mnemonic vmovaps --case xmm-load-u1 \
    --output-dir "$output_dir"

python3 - "$output_dir/vmovaps.json" <<'PY'
import json
import sys

result = json.load(open(sys.argv[1]))
assert result["status"] == "pass_x86_stdout_status", result
assert result["runs"]["x86"]["signal"] == 11
assert result["runs"]["x86"]["signal_code"] == 128
assert result["runs"]["lasx"]["signal"] == 11
assert result["runs"]["lasx"]["signal_code"] == 128
assert result["runs"]["lsx"]["signal"] == 11
assert result["runs"]["lsx"]["signal_code"] == 128
assert result["runs"]["lsx"]["option_enable_lasx_readback"] == 0
assert result["runs"]["lsx"]["option_enable_lasx_readback_confirmed"] is True
print("PASS WI-1838 vmovaps alignment fault: LSX option_enable_lasx=0; x86/LASX/LSX SIGSEGV si_code=128")
PY
