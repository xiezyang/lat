#!/bin/sh
set -eu

repo_root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
file=$repo_root/target/i386/latx/translator/tr-avx-mov.c

body=$(awk '
    /^bool translate_vmovq\(IR1_INST \* pir1\)/ { found = 1 }
    found { print }
    found && /^}/ { exit }
' "$file")

if ! printf '%s\n' "$body" | grep -q 'if (1)' ||
   ! printf '%s\n' "$body" | grep -q '/\* LSX-only path \*/' ||
   ! printf '%s\n' "$body" | grep -q '/\* Original LASX path \*/'; then
    echo "FAIL translate_vmovq does not preserve the required branch layout" >&2
    exit 1
fi

lsx_path=$(printf '%s\n' "$body" | awk '
    /\/\* LSX-only path \*\// { found = 1; next }
    /\/\* Original LASX path \*\// { exit }
    found { print }
')
if [ -z "$lsx_path" ] || printf '%s\n' "$lsx_path" | grep -Eq '\bla_xv'; then
    echo "FAIL translate_vmovq reachable path contains LASX" >&2
    exit 1
fi
for required in la_vxor_v la_vinsgr2vr_d la_vpickve2gr_du \
                load_u64_from_ir1_mem_exact store_u64_to_ir1_mem_exact \
                clear_ymm_high128_shadow; do
    if ! printf '%s\n' "$lsx_path" | grep -q "$required"; then
        echo "FAIL translate_vmovq reachable path misses $required" >&2
        exit 1
    fi
done

echo "PASS translate_vmovq selects an LSX/integer-only path"
