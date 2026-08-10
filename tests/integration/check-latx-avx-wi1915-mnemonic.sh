#!/bin/sh
set -eu
[ "$#" -eq 2 ] || exit 2
probe=$1
expected=$2
tmpdir=$(mktemp -d /tmp/wi1915-static.XXXXXX)
case "$tmpdir" in /tmp/*) ;; *) exit 2 ;; esac
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
objdump -d --no-show-raw-insn -Mintel "$probe" > "$tmpdir/disassembly"
mnemonics=$(awk '$2 ~ /^v[a-zA-Z0-9_]+$/ {print $2}' "$tmpdir/disassembly" | sort -u)
expected_set=$(printf '%s\nvmovdqu\n' "$expected" | sort -u)
[ "$mnemonics" = "$expected_set" ] || { echo "FAIL $expected found=$mnemonics" >&2; exit 1; }
case "$expected" in
    vphminposuw) count=5 ;;
    vpmovmskb) count=8 ;;
    *) count=9 ;;
esac
actual=$(grep -Ec "[[:space:]]$expected[[:space:]]" "$tmpdir/disassembly" || true)
[ "$actual" -eq "$count" ] || { echo "FAIL $expected count=$actual expected=$count" >&2; exit 1; }
echo "PASS WI-1915 single mnemonic: $expected count=$actual"
