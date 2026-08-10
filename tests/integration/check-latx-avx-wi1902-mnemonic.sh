#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 STATIC_X86_PROBE" >&2
    exit 2
fi
probe=$1
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
objdump -d --no-show-raw-insn -Mintel "$probe" >"$tmpdir/disassembly"
if grep -Eq '^[[:space:]]*[0-9a-f]+:[[:space:]]+62[[:space:]]' "$tmpdir/disassembly"; then
    echo "FAIL EVEX encoding is outside the supported VEX test range" >&2
    exit 1
fi
mnemonics=$(awk '$2 ~ /^v[a-zA-Z0-9_]+$/ { print $2 }' "$tmpdir/disassembly" | sort -u)
expected=$(printf '%s\n' vmovdqu vmovmskpd vmovmskps vzeroupper | sort -u)
[ "$mnemonics" = "$expected" ] || {
    echo "FAIL VMOVMSK mnemonic set: $mnemonics" >&2
    exit 1
}
if grep -Eq 'vmovmsk(ps|pd)[[:space:]].*ptr' "$tmpdir/disassembly"; then
    echo "FAIL VMOVMSK has an invalid memory-source encoding" >&2
    exit 1
fi
for mnemonic in vmovmskps vmovmskpd; do
    count=$(grep -c "[[:space:]]$mnemonic[[:space:]]" "$tmpdir/disassembly" || true)
    [ "$count" -ge 4 ] || {
        echo "FAIL $mnemonic instruction count: $count" >&2
        exit 1
    }
done
echo 'PASS WI-1902 x86 VMOVMSK mnemonic and no-memory-source check'
