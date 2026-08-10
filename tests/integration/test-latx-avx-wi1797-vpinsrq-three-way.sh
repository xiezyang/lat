#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
    echo "usage: $0 LATX PROBE EXPECTED_DIR OUTPUT_DIR" >&2
    exit 2
fi

latx=$1
probe=$2
expected=$3
output=$4
script_dir=$(cd "$(dirname "$0")" && pwd)
source_file=$(cd "$script_dir/../.." && pwd)/target/i386/latx/translator/tr-avx.c
mkdir -p "$output/lasx" "$output/lsx"

"$script_dir/check-latx-avx-single-mnemonic.sh" "$probe" vpinsrq

for name in normal fault-invalid fault-cross-page; do
    set +e
    if [[ "$name" == normal ]]; then
        env LATX_AVX_CPUID=1 "$latx" "$probe" \
            >"$output/lasx/$name.stdout" 2>"$output/lasx/$name.stderr"
    else
        env LATX_AVX_CPUID=1 "$latx" "$probe" "$name" \
            >"$output/lasx/$name.stdout" 2>"$output/lasx/$name.stderr"
    fi
    rc=$?
    set -e
    printf '%s\n' "$rc" >"$output/lasx/$name.status"
done

cmp "$expected/normal.bin" "$output/lasx/normal.stdout"
printf '0\n' | cmp - "$output/lasx/normal.status"
for name in fault-invalid fault-cross-page; do
    cmp "$expected/$name.bin" "$output/lasx/$name.stdout"
    cmp "$expected/$name.status" "$output/lasx/$name.status"
done

option_address=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1 }')
[[ -n "$option_address" ]]
for name in normal fault-invalid fault-cross-page; do
    if [[ "$name" == normal ]]; then
        args=()
    else
        args=("$name")
    fi
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'break translate_context_init' \
        -ex "run $probe ${args[*]} > $output/lsx/$name.stdout 2> $output/lsx/$name.stderr" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue >"$output/lsx/$name.gdb.log" 2>&1 || true
    grep -Eq '<option_enable_lasx>:[[:space:]]+0$' \
        "$output/lsx/$name.gdb.log"
    cmp "$expected/$name.bin" "$output/lsx/$name.stdout"
    if [[ "$name" == normal ]]; then
        grep -Eq 'exited normally' "$output/lsx/$name.gdb.log"
    else
        grep -Eq 'exited with code 0213' "$output/lsx/$name.gdb.log"
    fi
done

python3 - "$source_file" <<'PY'
import sys

text = open(sys.argv[1]).read()
start = text.index("bool translate_vpinsrq(IR1_INST")
end = text.index("bool translate_xgetbv", start)
body = text[start:end]
assert "load_u64_from_ir1_mem_exact(opnd2)" in body
print("PASS VPINSRQ LASX exact memory source contract")
PY

echo "PASS VPINSRQ x86/LASX/LSX normal and fault-cross-page regression"
