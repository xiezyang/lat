#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 LATX_X86_64 STATIC_X86_PROBE X86_REFERENCE" >&2
    exit 2
fi

latx=$1
probe=$2
reference=$3
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

if [ ! -x "$latx" ] || [ ! -x "$probe" ] || [ ! -f "$reference" ]; then
    echo "FAIL LATX, probe, or x86 reference is missing" >&2
    exit 2
fi

"$(dirname "$0")/check-latx-avx-single-mnemonic.sh" "$probe" vmovq

actual=$tmpdir/latx-vmovq.out
env LATX_AVX_CPUID=0 "$latx" "$probe" >"$actual"
if ! cmp -s "$reference" "$actual"; then
    echo "FAIL vmovq x86/LATX output differs" >&2
    cmp -l "$reference" "$actual" | sed -n '1,40p' >&2 || true
    exit 1
fi

echo "PASS vmovq x86/LATX differential"
sha256sum "$reference" "$actual"
