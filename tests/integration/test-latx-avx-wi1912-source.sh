#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
avx_source="$repo_dir/target/i386/latx/translator/tr-avx.c"
opnd_source="$repo_dir/target/i386/latx/translator/tr-opnd-process.c"
translate_source="$repo_dir/target/i386/latx/translator/translate.c"
header="$repo_dir/target/i386/latx/include/translate.h"

git -C "$repo_dir" diff --check

for name in \
    vbroadcastf128 vbroadcasti128 vbroadcastsd vbroadcastss \
    vpbroadcastb vpbroadcastd vpbroadcastq vpbroadcastw \
    vextractf128 vextracti128 vextractps \
    vinsertf128 vinserti128 vinsertps \
    vpextrx vpinsrx; do
    grep -Fq "translate_${name}_lsx" "$avx_source"
    grep -Fq "TRANS_FUNC_DEF(${name}_lsx)" "$header"
done

for name in VBROADCASTF128 VBROADCASTI128 VBROADCASTSD VBROADCASTSS \
    VPBROADCASTB VPBROADCASTD VPBROADCASTQ VPBROADCASTW VEXTRACTF128 \
    VEXTRACTI128 VEXTRACTPS VINSERTF128 VINSERTI128 VINSERTPS VPEXTRB \
    VPEXTRD VPEXTRQ VPEXTRW VPINSRB VPINSRD VPINSRW; do
    grep -Fq "dt_X86_INS_${name}" "$translate_source"
done

grep -Fq 'load_u8_from_ir1_mem_exact' "$opnd_source"
grep -Fq 'load_u16_from_ir1_mem_exact' "$opnd_source"
grep -Fq 'check_guest_mem_range(address, 16, PAGE_READ)' "$opnd_source"
grep -Fq 'check_guest_mem_range(address, 32, PAGE_READ)' "$opnd_source"

lsx_block=$(sed -n '1933,2020p' "$translate_source")
for name in VBROADCASTF128 VBROADCASTI128 VBROADCASTSD VBROADCASTSS \
    VPBROADCASTB VPBROADCASTD VPBROADCASTQ VPBROADCASTW VEXTRACTF128 \
    VEXTRACTI128 VEXTRACTPS VINSERTF128 VINSERTI128 VINSERTPS VPEXTRB \
    VPEXTRD VPEXTRQ VPEXTRW VPINSRB VPINSRD VPINSRW; do
    printf '%s\n' "$lsx_block" | grep -Fq "dt_X86_INS_${name}"
done

if printf '%s\n' "$lsx_block" | grep -Eq 'la_xv'; then
    echo "FAIL WI-1912 registration block references LASX" >&2
    exit 1
fi

echo "PASS WI-1912 source, exact-memory and option gate checks"
