#!/bin/sh
set -eu

if [ "$#" -ne 3 ] || [ "$1" != static-check ]; then
    echo "usage: $0 static-check PROBE SOURCE" >&2
    exit 2
fi
script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
probe=$2
source=$3
"$script_dir/check-latx-avx-single-mnemonic.sh" "$probe" vlddqu
lsx=$(awk '/bool translate_vlddqu_lsx\(/ {f=1} f {print} f&&/^}/ {exit}' "$source")
printf '%s\n' "$lsx" | grep -Fq 'return translate_vmovups_lsx'
if printf '%s\n' "$lsx" | grep -Eq '\bla_xv'; then
    echo "FAIL vlddqu LSX wrapper contains LASX generator" >&2
    exit 1
fi
lasx=$(awk '/bool translate_vlddqu\(/ {f=1} f {print} f&&/^}/ {exit}' "$source")
printf '%s\n' "$lasx" | grep -Fq 'translate_vmovaps_lasx'
echo "PASS VLDDQU source and single-mnemonic static contract"
