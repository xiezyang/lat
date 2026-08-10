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

if [ ! -x "$probe" ]; then
    echo "FAIL probe is not executable: $probe" >&2
    exit 2
fi

disassembly=$tmpdir/objdump.txt
objdump -d -Mintel "$probe" >"$disassembly"

if grep -Eq '^[[:space:]]*[0-9a-f]+:[[:space:]]+62[[:space:]]' \
    "$disassembly"; then
    echo "FAIL EVEX encoding is outside the supported VEX test range" >&2
    exit 1
fi

mnemonics=$(objdump -d --no-show-raw-insn -Mintel "$probe" |
    awk '$2 ~ /^v[a-zA-Z0-9_]+$/ { print $2 }' | sort -u)
if [ "$mnemonics" != "$expected" ]; then
    echo "FAIL expected AVX mnemonic '$expected', found: $mnemonics" >&2
    exit 1
fi

echo "PASS single AVX mnemonic: $expected"
