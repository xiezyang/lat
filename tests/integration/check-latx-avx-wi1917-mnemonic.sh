#!/bin/sh
set -eu
[ "$#" -eq 2 ] || exit 2
probe=$1
expected=$2
tmpdir=$(mktemp -d /tmp/wi1917-static.XXXXXX)
case "$tmpdir" in /tmp/*) ;; *) exit 2 ;; esac
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
objdump -d --no-show-raw-insn -Mintel "$probe" > "$tmpdir/disassembly"
mnemonics=$(awk '$2 ~ /^v[a-zA-Z0-9_]+$/ {print $2}' "$tmpdir/disassembly" | sort -u)
expected_set=$(printf '%s\nvmovdqu\n' "$expected" | sort -u)
[ "$mnemonics" = "$expected_set" ] || { echo "FAIL $expected found=$mnemonics" >&2; exit 1; }
count=$(grep -Ec "[[:space:]]$expected[[:space:]]" "$tmpdir/disassembly" || true)
case "$expected" in vpgatherqd|vgatherqps) expected_count=3 ;; *) expected_count=5 ;; esac
[ "$count" -eq "$expected_count" ] || { echo "FAIL $expected count=$count expected=$expected_count" >&2; exit 1; }
echo "PASS WI-1917 single mnemonic: $expected count=$count"
