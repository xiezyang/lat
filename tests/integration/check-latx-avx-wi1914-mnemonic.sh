#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 STATIC_X86_PROBE EXPECTED_AVX_MNEMONIC" >&2
    exit 2
fi
probe=$1
expected=$2
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
objdump -d --no-show-raw-insn -Mintel "$probe" > "$tmpdir/disassembly"
mnemonics=$(awk '$2 ~ /^v[a-zA-Z0-9_]+$/ { print $2 }' "$tmpdir/disassembly" | sort -u)
expected_set=$(printf '%s\nvmovdqu\nvzeroupper\n%s\n' "$expected" \
    "" | sed '/^$/d' | sort -u)
[ "$mnemonics" = "$expected_set" ] || {
    echo "FAIL $expected mnemonic set: $mnemonics" >&2
    exit 1
}
count=$(grep -Ec "[[:space:]]$expected[[:space:]]" "$tmpdir/disassembly" || true)
[ "$count" -ge 12 ] || {
    echo "FAIL $expected normal instruction count: $count" >&2
    exit 1
}
echo "PASS WI-1914 single mnemonic: $expected"
