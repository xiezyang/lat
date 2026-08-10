#!/bin/sh

set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 LATX_X86_64 STATIC_X86_PROBE OUTPUT_FILE" >&2
    exit 2
fi

latx=$1
probe=$2
output=$3

env LATX_AVX_CPUID=0 "$latx" "$probe" >"$output" 2>&1

if grep -q '^FAIL ' "$output"; then
    sed -n '1,160p' "$output" >&2
    exit 1
fi

pass_count=$(grep -c '^PASS ' "$output")
if [ "$pass_count" -ne 21 ]; then
    echo "FAIL expected 21 semantic checks, got $pass_count" >&2
    sed -n '1,160p' "$output" >&2
    exit 1
fi

if ! grep -q '^RESULT PASS all six AVX instruction semantics$' "$output"; then
    echo "FAIL final semantic result is missing" >&2
    sed -n '1,160p' "$output" >&2
    exit 1
fi

sed -n '/^PASS /,$p' "$output"
