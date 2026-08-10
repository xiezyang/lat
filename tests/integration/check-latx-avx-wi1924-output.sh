#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 NORMAL_OUTPUT" >&2
    exit 2
fi
output=$1
tmpdir=$(mktemp -d /tmp/wi1924-output-check-XXXXXX)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
test "$(wc -c < "$output")" -eq 2112

if dd if="$output" bs=32 skip=16 count=16 2>/dev/null |
    od -An -v -tx1 | grep -Eq '[1-9a-fA-F]'; then
    echo "FAIL VZEROALL snapshot contains nonzero vector bytes" >&2
    exit 1
fi

for i in $(seq 0 15); do
    dd if="$output" bs=32 skip="$i" count=1 2>/dev/null |
        dd of="$tmpdir/pre-$i" bs=1 count=16 2>/dev/null
    dd if="$output" bs=32 skip=$((32 + i)) count=1 2>/dev/null |
        dd of="$tmpdir/upper-low-$i" bs=1 count=16 2>/dev/null
    cmp "$tmpdir/pre-$i" "$tmpdir/upper-low-$i"
    if dd if="$output" bs=32 skip=$((32 + i)) count=1 2>/dev/null |
        dd bs=1 skip=16 count=16 2>/dev/null | od -An -v -tx1 |
        grep -Eq '[1-9a-fA-F]'; then
        echo "FAIL VZEROUPPER high half register=$i" >&2
        exit 1
    fi
done

dd if="$output" bs=1 skip=1536 count=128 2>/dev/null >"$tmpdir/gpr-zero-before"
dd if="$output" bs=1 skip=1664 count=128 2>/dev/null >"$tmpdir/gpr-zero-after"
dd if="$output" bs=1 skip=1792 count=128 2>/dev/null >"$tmpdir/gpr-upper-before"
dd if="$output" bs=1 skip=1920 count=128 2>/dev/null >"$tmpdir/gpr-upper-after"
cmp "$tmpdir/gpr-zero-before" "$tmpdir/gpr-zero-after"
cmp "$tmpdir/gpr-upper-before" "$tmpdir/gpr-upper-after"

for pair in "0 8" "16 20" "24 32" "40 44"; do
    set -- $pair
    count=$(( $2 - $1 ))
    dd if="$output" bs=1 skip=$((2048 + $1)) count="$count" 2>/dev/null >"$tmpdir/state-before"
    dd if="$output" bs=1 skip=$((2048 + $2)) count="$count" 2>/dev/null >"$tmpdir/state-after"
    cmp "$tmpdir/state-before" "$tmpdir/state-after"
done
echo "PASS WI-1924 vector and scalar state output"
