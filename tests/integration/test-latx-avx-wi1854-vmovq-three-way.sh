#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
    echo "usage: $0 LATX PROBE X86_REFERENCE OUTPUT_DIR" >&2
    exit 2
fi

latx=$1
probe=$2
x86_reference=$3
output=$4
script_dir=$(cd "$(dirname "$0")" && pwd)
source_file=$(cd "$script_dir/../.." && pwd)/target/i386/latx/translator/tr-avx-mov.c
mkdir -p "$output"

lasx_diff=0
lsx_diff=0

"$script_dir/check-latx-avx-single-mnemonic.sh" "$probe" vmovq

set +e
LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" \
    >"$output/lasx.stdout" 2>"$output/lasx.stderr"
lasx_rc=$?
set -e
printf '%s\n' "$lasx_rc" >"$output/lasx.status"
if [[ "$lasx_rc" != 0 ]] || ! cmp "$x86_reference" "$output/lasx.stdout"; then
    lasx_diff=1
    echo "DIFF LASX all_existing_fixture_cases" >>"$output/test-output.txt"
fi

addr=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1 }')
[[ -n "$addr" ]]
gdb -q -batch "$latx" \
    -ex 'set pagination off' \
    -ex 'handle SIGSEGV pass nostop noprint' \
    -ex 'handle SIGBUS pass nostop noprint' \
    -ex 'break translate_context_init' \
    -ex 'set environment LATX_AVX_CPUID 1' \
    -ex 'set environment LATX_AOT 0' \
    -ex "run $probe > $output/lsx.stdout 2> $output/lsx.stderr" \
    -ex "set {int}$addr = 0" \
    -ex "x/wd $addr" \
    -ex continue >"$output/lsx.gdb.log" 2>&1 || true
grep -Eq 'option_enable_lasx.*:[[:space:]]+0$' "$output/lsx.gdb.log"
if ! grep -Eq 'exited normally' "$output/lsx.gdb.log" || ! cmp "$x86_reference" "$output/lsx.stdout"; then
    lsx_diff=1
    echo "DIFF LSX all_existing_fixture_cases" >>"$output/test-output.txt"
fi
if grep -Eq 'exited normally' "$output/lsx.gdb.log"; then
    printf '0\n' >"$output/lsx.status"
else
    printf '127\n' >"$output/lsx.status"
fi

sha256sum "$probe" "$x86_reference" "$latx" "$source_file" \
    "$output/lasx.stdout" "$output/lsx.stdout" >"$output/hashes.sha256"

python3 - "$output" "$probe" "$x86_reference" "$latx" "$source_file" "$lasx_diff" "$lsx_diff" <<'PY'
import hashlib
import json
import pathlib
import sys

out = pathlib.Path(sys.argv[1])
probe, reference, latx, source = map(pathlib.Path, sys.argv[2:6])
lasx_diff = int(sys.argv[6])
lsx_diff = int(sys.argv[7])

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

record = {
    "work_item": "WI-1854",
    "mnemonic": "vmovq",
    "baseline": "x86-native-only",
    "fixture": {"probe": str(probe), "sha256": sha(probe)},
    "source": {"path": str(source), "sha256": sha(source)},
    "binary": {"path": str(latx), "sha256": sha(latx)},
    "cases": {
        "all_existing_fixture_cases": {
            "x86": {"hash": sha(reference), "exit_code": 0},
            "lasx": {"hash": sha(out / "lasx.stdout"), "exit_code": int((out / "lasx.status").read_text()), "comparison": "match" if sha(reference) == sha(out / "lasx.stdout") else "diff"},
            "lsx": {"hash": sha(out / "lsx.stdout"), "exit_code": int((out / "lsx.status").read_text()), "comparison": "match" if sha(reference) == sha(out / "lsx.stdout") else "diff"},
        }
    },
    "lsx_option_enable_lasx_readback": 0,
    "commands": [
        "check-latx-avx-single-mnemonic.sh <probe> vmovq",
        "LATX_AVX_CPUID=1 LATX_AOT=0 <latx> <probe>",
        "gdb -q -batch <latx> -ex 'set environment LATX_AOT 0' -ex 'set {int}<option_enable_lasx>=0' -ex continue",
    ],
    "classification": "passed: LASX and LSX match x86" if not (lasx_diff or lsx_diff) else ("B: LASX differs from x86; LSX matches x86" if not lsx_diff else "B/C: LSX also differs from x86"),
    "lasx_diff": bool(lasx_diff),
    "lsx_diff": bool(lsx_diff),
}
(out / "result.json").write_text(json.dumps(record, indent=2) + "\n")
PY

cat >"$output/test-output.txt" <<EOF
WI-1854 VMOVQ three-way acceptance
PASS single AVX mnemonic: vmovq
PASS LSX option_enable_lasx=0 readback
EOF
if [[ "$lasx_diff" != 0 ]]; then echo "B LASX differs from x86" >>"$output/test-output.txt"; fi
if [[ "$lsx_diff" != 0 ]]; then echo "B/C LSX differs from x86" >>"$output/test-output.txt"; fi
if [[ "$lasx_diff" == 0 && "$lsx_diff" == 0 ]]; then
    echo "PASS VMOVQ x86/LASX/forced-LSX regression with LATX_AOT=0" >>"$output/test-output.txt"
fi
cat "$output/test-output.txt"

if [[ "$lasx_diff" != 0 || "$lsx_diff" != 0 ]]; then
    exit 1
fi
