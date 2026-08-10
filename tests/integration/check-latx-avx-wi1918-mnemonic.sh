#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 STATIC_X86_PROBE EXPECTED_AVX_MNEMONIC" >&2
    exit 2
fi

probe=$1
expected=$2
tmpdir=$(mktemp -d /tmp/wi1918-check-XXXXXX)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

[ -x "$probe" ] || { echo "FAIL probe is not executable: $probe" >&2; exit 2; }
objdump -d -Mintel "$probe" >"$tmpdir/objdump.txt"
if grep -Eq '^[[:space:]]*[0-9a-f]+:[[:space:]]+62[[:space:]]' "$tmpdir/objdump.txt"; then
    echo "FAIL EVEX encoding is outside the AVX VEX fixture boundary" >&2
    exit 1
fi

mnemonics=$(objdump -d --no-show-raw-insn -Mintel "$probe" |
    awk '$2 ~ /^v[a-zA-Z0-9_]+$/ { print tolower($2) }' | sort -u)
allowed=$(printf '%s\n' "$expected" vmovdqu vpxor vzeroupper | sort -u)
if [ "$(printf '%s\n' "$mnemonics" | sort -u)" != "$allowed" ]; then
    echo "FAIL expected $expected plus save instructions, found: $mnemonics" >&2
    exit 1
fi
grep -Eq "[[:space:]]$expected([[:space:]]|$)" "$tmpdir/objdump.txt"
echo "PASS WI-1918 single AVX mnemonic: $expected"
