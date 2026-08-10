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
[ "$(grep -Ec "[[:space:]]$expected[[:space:]]" "$tmpdir/disassembly" || true)" -eq 6 ] || {
    echo "FAIL expected six $expected instructions" >&2
    exit 1
}
if grep -E "[[:space:]]$expected[[:space:]].*YMMWORD" "$tmpdir/disassembly"; then
    echo "FAIL $expected emitted a YMM operand; only XMM and m64 are legal" >&2
    exit 1
fi
echo "PASS WI-1904 x86 fixture mnemonic: $expected"
