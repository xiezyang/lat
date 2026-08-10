#!/bin/sh
set -eu

usage()
{
    echo "usage: $0 LATX_X86_64 PROBE EXPECTED_DIR" >&2
    exit 2
}

[ "$#" -eq 3 ] || usage
latx=$1
probe=$2
expected=$3
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

env LATX_AVX_CPUID=1 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
    LATX_AVX_TRACE_YMM_INIT=0 \
    "$latx" "$probe" reference >"$tmpdir/lasx.bin" \
    2>"$tmpdir/lasx.err"

option_address=$(nm -g "$latx" | awk '$3 == "option_enable_lasx" {
    print "0x" $1
}')
[ -n "$option_address" ] || {
    echo "FAIL option_enable_lasx symbol is missing" >&2
    exit 1
}
gdb -q -batch "$latx" \
    -ex 'set pagination off' \
    -ex 'set environment LATX_AVX_CPUID 1' \
    -ex 'set environment LATX_AVX_TRACE 3' \
    -ex 'set environment LATX_AVX_TRACE_YMM 15' \
    -ex 'set environment LATX_AVX_TRACE_YMM_INIT 0' \
    -ex 'break translate_context_init' \
    -ex "run $probe reference > $tmpdir/lsx.bin 2> $tmpdir/lsx.err" \
    -ex "set {int}$option_address = 0" \
    -ex "x/wd $option_address" \
    -ex continue >"$tmpdir/lsx.gdb" 2>&1
grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$tmpdir/lsx.gdb"

[ "$(wc -c <"$tmpdir/lasx.bin")" -eq 23680 ]
[ "$(wc -c <"$tmpdir/lsx.bin")" -eq 23680 ]
cmp "$expected/reference.bin" "$tmpdir/lasx.bin"
cmp "$expected/reference.bin" "$tmpdir/lsx.bin"

printf 'PASS XSAVE AVX high-128 export: LASX and LSX match x86 (%s bytes)\n' \
    "$(wc -c <"$tmpdir/lasx.bin")"
