#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 LATX_X86_64 STATIC_X86_PROBE" >&2
    exit 2
fi

latx=$1
probe=$2
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

if [ ! -x "$latx" ] || [ ! -x "$probe" ]; then
    echo "FAIL LATX and probe must be executable" >&2
    exit 2
fi

mnemonics=$(objdump -d --no-show-raw-insn -Mintel "$probe" |
    awk '$2 ~ /^v/ { print $2 }' | sort -u)
if [ "$mnemonics" != "vmovdqa" ]; then
    echo "FAIL probe AVX mnemonics: $mnemonics" >&2
    exit 1
fi

output=$tmpdir/trace.log
env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=4 \
    "$latx" "$probe" >"$output" 2>&1

expected='count=2 reg=4 low0=0706050403020100 low1=0f0e0d0c0b0a0908'
if ! grep -Fq "$expected" "$output"; then
    echo "FAIL low 128-bit state" >&2
    sed -n '1,160p' "$output" >&2
    exit 1
fi
expected='shadow_high0=1716151413121110 shadow_high1=1f1e1d1c1b1a1918'
if ! grep -Fq "$expected" "$output"; then
    echo "FAIL shadow high 128-bit state" >&2
    sed -n '1,160p' "$output" >&2
    exit 1
fi

disabled=$tmpdir/disabled.log
env LATX_AVX_CPUID=0 LATX_AVX_TRACE=0 LATX_AVX_TRACE_YMM=4 \
    "$latx" "$probe" >"$disabled" 2>&1
if grep -Fq 'event=ymm_state' "$disabled"; then
    echo "FAIL YMM state trace is not disabled" >&2
    sed -n '1,160p' "$disabled" >&2
    exit 1
fi

invalid=$tmpdir/invalid.log
set +e
env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=16 \
    "$latx" "$probe" >"$invalid" 2>&1
status=$?
set -e
if [ "$status" -eq 0 ] ||
   ! grep -Fq 'LATX_AVX_TRACE_YMM register is out of range' "$invalid"; then
    echo "FAIL invalid YMM register was accepted" >&2
    sed -n '1,160p' "$invalid" >&2
    exit 1
fi

echo "PASS CPUID0 YMM state trace"
grep -F 'event=ymm_state' "$output"
