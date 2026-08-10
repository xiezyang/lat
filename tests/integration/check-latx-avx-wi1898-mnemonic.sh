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
if grep -Eq '^[[:space:]]*[0-9a-f]+:[[:space:]]+62[[:space:]]' "$tmpdir/disassembly"; then
    echo "FAIL EVEX encoding is outside the supported VEX test range" >&2
    exit 1
fi
mnemonics=$(awk '$2 ~ /^v[a-zA-Z0-9_]+$/ { print $2 }' "$tmpdir/disassembly" | sort -u)
expected_set=$(printf '%s\nvmovdqu\nvpxor\nvzeroupper\n' "$expected" | sort -u)
if [ "$mnemonics" != "$expected_set" ]; then
    echo "FAIL expected $expected plus fixture setup mnemonics, found:" >&2
    printf '%s\n' "$mnemonics" >&2
    exit 1
fi
count=$(grep -c "[[:space:]]$expected[[:space:]]" "$tmpdir/disassembly" || true)
[ "$count" -ge 10 ] || {
    echo "FAIL expected at least 10 $expected instructions, found $count" >&2
    exit 1
}
echo "PASS WI-1898 x86 fixture mnemonic: $expected"
