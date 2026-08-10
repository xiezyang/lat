#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
manifest=$root/tests/integration/latx-avx-wi1914-mnemonics.txt
generator=$root/tests/integration/generate-latx-avx-wi1914-fixtures.py
fixture_dir=$root/tests/integration/wi1914-fixtures

test -s "$manifest"
test -f "$generator"
count=0
while read -r mnemonic; do
    test -n "$mnemonic"
    asm=$fixture_dir/latx-avx-single-$mnemonic.S
    c=$fixture_dir/latx-avx-single-$mnemonic.c
    test -f "$asm"
    test -f "$c"
    grep -q "latx_avx_single_${mnemonic}_run" "$asm"
    grep -q "latx_avx_single_${mnemonic}_fault_xmm" "$asm"
    grep -q "${mnemonic} " "$asm"
    grep -q '0x8000000000000000' "$asm"
    grep -q '0x7ff8000000000042' "$asm"
    grep -q '0x7ff0000000000001' "$asm"
    if grep -q "latx_avx_single_${mnemonic}_fault_ymm" "$asm"; then
        grep -q 'ymmword ptr \[rdi\]' "$asm"
    fi
    count=$((count + 1))
done < "$manifest"

test "$count" -eq 46
printf 'PASS WI-1914 fixture source audit: count=%s, independent XMM/scalar/packed forms and fault entrypoints\n' "$count"
