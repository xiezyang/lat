#!/bin/sh
set -eu
[ "$#" -eq 2 ] || exit 2
probe=$1
tmpdir=$(mktemp -d /tmp/wi1794-vpandn-check-XXXXXX)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
objdump -d -Mintel "$probe" >"$tmpdir/objdump.txt"
if grep -Eq '^[[:space:]]*[0-9a-f]+:[[:space:]]+62[[:space:]]' "$tmpdir/objdump.txt"; then exit 1; fi
mnemonics=$(objdump -d --no-show-raw-insn -Mintel "$probe" | awk '$2 ~ /^v[a-zA-Z0-9_]+$/ {print tolower($2)}' | sort -u)
allowed=$(printf '%s\n' vmovdqu vpandn vzeroupper | sort -u)
test "$(printf '%s\n' "$mnemonics" | sort -u)" = "$allowed"
grep -Eq '[[:space:]]vpandn([[:space:]]|$)' "$tmpdir/objdump.txt"
echo 'PASS WI-1794 VPANDN x86 static fixture'
