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
if [ "$mnemonics" != "vzeroupper" ]; then
    echo "FAIL probe AVX mnemonics: $mnemonics" >&2
    exit 1
fi

output=$tmpdir/trace.log
env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
    LATX_AVX_TRACE_YMM_INIT=1 "$latx" "$probe" >"$output" 2>&1

expected='event=ymm_init'
if ! grep -Fq "$expected" "$output"; then
    echo "FAIL YMM initial state was not injected" >&2
    sed -n '1,160p' "$output" >&2
    exit 1
fi
expected='count=1 reg=15 low0=0000000000000000 low1=0000000000000000'
if ! grep -Fq "$expected" "$output"; then
    echo "FAIL first YMM state record" >&2
    sed -n '1,160p' "$output" >&2
    exit 1
fi
expected='shadow_high0=1122334455667788 shadow_high1=99aabbccddeeff00'
if ! grep -Fq "$expected" "$output"; then
    echo "FAIL injected YMM high half" >&2
    sed -n '1,160p' "$output" >&2
    exit 1
fi
expected='count=2 reg=15 low0=0000000000000000 low1=0000000000000000'
if ! grep -Fq "$expected" "$output"; then
    echo "FAIL second YMM state record" >&2
    sed -n '1,160p' "$output" >&2
    exit 1
fi
second=$(grep -F 'event=ymm_state' "$output" | sed -n '2p')
case "$second" in
    *'shadow_high0=0000000000000000 shadow_high1=0000000000000000'*) ;;
    *)
        echo "FAIL vzeroupper did not clear the injected high half" >&2
        sed -n '1,160p' "$output" >&2
        exit 1
        ;;
esac

disabled=$tmpdir/disabled.log
env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
    "$latx" "$probe" >"$disabled" 2>&1
if grep -Fq 'event=ymm_init' "$disabled"; then
    echo "FAIL YMM initialization is not disabled by default" >&2
    sed -n '1,160p' "$disabled" >&2
    exit 1
fi

invalid=$tmpdir/invalid.log
set +e
env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
    LATX_AVX_TRACE_YMM_INIT=2 "$latx" "$probe" >"$invalid" 2>&1
status=$?
set -e
if [ "$status" -eq 0 ] ||
   ! grep -Fq 'LATX_AVX_TRACE_YMM_INIT must be 0 or 1' "$invalid"; then
    echo "FAIL invalid YMM initialization option was accepted" >&2
    sed -n '1,160p' "$invalid" >&2
    exit 1
fi

missing=$tmpdir/missing-register.log
set +e
env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM_INIT=1 \
    "$latx" "$probe" >"$missing" 2>&1
status=$?
set -e
if [ "$status" -eq 0 ] ||
   ! grep -Fq 'LATX_AVX_TRACE_YMM_INIT requires LATX_AVX_TRACE_YMM' "$missing"; then
    echo "FAIL missing YMM register was accepted" >&2
    sed -n '1,160p' "$missing" >&2
    exit 1
fi

echo "PASS CPUID0 YMM initial state injection"
grep -F 'event=ymm_' "$output"
