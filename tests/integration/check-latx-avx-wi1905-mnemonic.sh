#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 STATIC_X86_PROBE EXPECTED_AVX_MNEMONIC" >&2
    exit 2
fi

probe=$1
expected=$2
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
objdump -d --no-show-raw-insn -Mintel "$probe" >"$tmpdir/disassembly"
mnemonics=$(awk '$2 ~ /^v[a-zA-Z0-9_]+$/ { print $2 }' "$tmpdir/disassembly" | sort -u)
expected_set=$(printf '%s\nvmovdqu\nvzeroupper\n' "$expected" | sort -u)
[ "$mnemonics" = "$expected_set" ] || {
    echo "FAIL expected only $expected plus VMOVDQU/VZEROUPPER, found:" >&2
    printf '%s\n' "$mnemonics" >&2
    exit 1
}
target_count=$(grep -Ec "[[:space:]]$expected[[:space:]]" "$tmpdir/disassembly" || true)
[ "$target_count" -eq 6 ] || {
    echo "FAIL expected four normal and two fault $expected instructions, found $target_count" >&2
    exit 1
}
grep -Eq "[[:space:]]$expected xmm[^,]*,[[:space:]]*xmm[^,]*,[[:space:]]*QWORD PTR" "$tmpdir/disassembly"
grep -Eq "[[:space:]]$expected QWORD PTR.*xmm" "$tmpdir/disassembly"
echo "PASS WI-1905 x86 fixture mnemonic: $expected"
