#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 5 || ${1:-} != verify ]]; then
    echo "usage: $0 verify LATX PROBE EXPECTED_DIR OUTPUT_DIR" >&2
    exit 2
fi

latx=$2
probe=$3
expected=$4
output=$5
source_file=$(cd "$(dirname "$0")/../.." && pwd)/target/i386/latx/translator/tr-avx.c
mkdir -p "$output"

extract_function() {
    local name=$1
    awk -v name="$name" '
        $0 ~ "bool " name "\\(" { capture = 1 }
        capture {
            print
            opens += gsub(/\{/, "{")
            closes += gsub(/\}/, "}")
            if (opens > 0 && opens == closes) exit
        }
    ' "$source_file"
}

lsx_body=$(extract_function translate_vdivsd_lsx)
lasx_body=$(extract_function translate_vdivsd)
printf '%s\n' "$lsx_body" | grep -Fq 'la_vfdiv_d'
printf '%s\n' "$lsx_body" | grep -Fq 'clear_ymm_high128_shadow'
if printf '%s\n' "$lsx_body" | grep -Eq '\bla_xv'; then
    echo 'FAIL VDIVSD LSX source contains LASX generator' >&2
    exit 1
fi
printf '%s\n' "$lasx_body" | grep -Fq 'la_fdiv_d'
printf '%s\n' "$lasx_body" | grep -Fq 'la_xvori_b'

addr=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1 }')
[[ -n "$addr" ]]

run_lasx() {
    local name=$1
    set +e
    LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" "$name" \
        >"$output/lasx-$name.bin" 2>"$output/lasx-$name.stderr"
    local rc=$?
    set -e
    printf '%s\n' "$rc" >"$output/lasx-$name.status"
}

run_lsx() {
    local name=$1
    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'set confirm off' \
        -ex 'handle SIGFPE pass nostop noprint' \
        -ex 'break translate_context_init' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        -ex "run $probe $name > $output/lsx-$name.bin 2> $output/lsx-$name.stderr" \
        -ex "set {int}$addr = 0" \
        -ex 'printf "option_enable_lasx_readback=%d\\n", *(int *)&option_enable_lasx' \
        -ex continue >"$output/lsx-$name.gdb.log" 2>&1
    set -e
    grep -Fq 'option_enable_lasx_readback=0' "$output/lsx-$name.gdb.log"
    if grep -Fq 'exited normally' "$output/lsx-$name.gdb.log"; then
        rc=0
    elif grep -Fq 'exited with code 0210' "$output/lsx-$name.gdb.log"; then
        rc=136
    elif grep -Fq 'exited with code 0132' "$output/lsx-$name.gdb.log"; then
        rc=90
    else
        rc=127
    fi
    printf '%s\n' "$rc" >"$output/lsx-$name.status"
}

run_lasx reference
run_lsx reference
cmp "$expected/reference.bin" "$output/lsx-reference.bin"
if ! cmp -s "$expected/reference.bin" "$output/lasx-reference.bin"; then
    echo 'INFO VDIVSD LASX differs from x86; preserving LASX deviation only'
fi
for name in fpe-invalid fpe-divzero fpe-precision; do
    run_lasx "$name"
    run_lsx "$name"
    cmp "$expected/$name.bin" "$output/lsx-$name.bin"
    cmp "$expected/$name.status" "$output/lsx-$name.status"
    if ! cmp -s "$expected/$name.bin" "$output/lasx-$name.bin" ||
        ! cmp -s "$expected/$name.status" "$output/lasx-$name.status"; then
        echo "INFO VDIVSD LASX differs from x86 for $name; preserving LASX deviation only"
    fi
done

sha256sum "$latx" "$probe" "$source_file" \
    "$expected/reference.bin" "$expected/fpe-invalid.bin" \
    "$expected/fpe-divzero.bin" "$expected/fpe-precision.bin" \
    "$output/lasx-reference.bin" "$output/lsx-reference.bin" \
    "$output/lasx-fpe-invalid.bin" "$output/lsx-fpe-invalid.bin" \
    "$output/lasx-fpe-divzero.bin" "$output/lsx-fpe-divzero.bin" \
    "$output/lasx-fpe-precision.bin" "$output/lsx-fpe-precision.bin" \
    >"$output/hashes.sha256"
echo 'PASS VDIVSD x86/LASX/forced-LSX three-way regression'
