#!/bin/sh
set -eu

latx_binary=${1:-build64/latx-x86_64}

if [ ! -x "$latx_binary" ]; then
    echo "FAIL LATX binary is not executable: $latx_binary" >&2
    exit 2
fi

dump=$(objdump -d --disassemble=translate_vmovq_lsx "$latx_binary")
if ! printf '%s\n' "$dump" | grep -q '<translate_vmovq_lsx>'; then
    echo "FAIL translate_vmovq_lsx symbol is missing" >&2
    exit 1
fi
if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>'; then
    echo "FAIL compiled translate_vmovq_lsx can call a LASX generator" >&2
    printf '%s\n' "$dump" | grep -E '<la_xv[^>]*>' >&2
    exit 1
fi

echo "PASS compiled translate_vmovq_lsx cannot call la_xv generators"
