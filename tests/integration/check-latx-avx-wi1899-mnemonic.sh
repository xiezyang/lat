#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 2 ]]; then echo "usage: $0 STATIC_X86_PROBE EXPECTED_AVX_MNEMONIC" >&2; exit 2; fi
probe=$1
expected=$2
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
objdump -d --no-show-raw-insn -Mintel "$probe" > "$tmpdir/disassembly"
if grep -Eq '^[[:space:]]*[0-9a-f]+:[[:space:]]+62[[:space:]]' "$tmpdir/disassembly"; then
    echo "FAIL EVEX encoding is outside the supported VEX test range" >&2
    exit 1
fi
actual=$(awk '$2 ~ /^v[a-zA-Z0-9_]+$/ { print $2 }' "$tmpdir/disassembly" | sort -u)
allowed=$(printf '%s\nvmovdqu\nvpxor\nvzeroupper\n' "$expected" | sort -u)
bad=$(comm -23 <(printf '%s\n' "$actual") <(printf '%s\n' "$allowed"))
[ -z "$bad" ] || { echo "FAIL unexpected VEX mnemonics: $bad" >&2; exit 1; }
printf '%s\n' "$actual" | grep -Fxq "$expected" || { echo "FAIL missing expected mnemonic: $expected" >&2; exit 1; }
echo "PASS WI-1899 x86 fixture mnemonic: $expected"
