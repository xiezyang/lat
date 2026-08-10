#!/usr/bin/env bash
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
actual=$(awk '$2 ~ /^v[a-zA-Z0-9_]+$/ { print $2 }' "$tmpdir/disassembly" | sort -u)
case "$expected" in
    vmovupd) allowed='vmovdqu\nvpxor\nvzeroupper\nvmovupd' ;;
    vmovups) allowed='vmovups' ;;
    *) echo "FAIL unsupported mnemonic: $expected" >&2; exit 2 ;;
esac
bad=$(comm -23 <(printf '%s\n' "$actual") <(printf '%b\n' "$allowed" | sort -u))
[ -z "$bad" ] || { echo "FAIL unexpected VEX mnemonics: $bad" >&2; exit 1; }
printf '%s\n' "$actual" | grep -Fxq "$expected" || {
    echo "FAIL missing expected mnemonic: $expected" >&2
    exit 1
}
echo "PASS WI-1897 x86 fixture mnemonic: $expected"
