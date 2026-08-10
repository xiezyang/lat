#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
source=$root/target/i386/latx/translator/tr-avx-mov.c
dispatch=$root/target/i386/latx/translator/translate.c
exact=$root/target/i386/latx/translator/tr-opnd-process.c
fixture_s=$root/tests/integration/latx-avx-single-vmovdqa.S
fixture_c=$root/tests/integration/latx-avx-single-vmovdqa.c

extract_function() {
    local name=$1
    awk -v name="$name" '
        $0 ~ "bool " name "\\(" { capture = 1 }
        capture {
            print
            opens += gsub(/\{/, "{")
            closes += gsub(/\}/, "}")
            if (opens > 0 && opens == closes) exit
        }
    ' "$source"
}

[[ -f "$source" && -f "$dispatch" && -f "$exact" && \
   -f "$fixture_s" && -f "$fixture_c" ]]
lsx=$(extract_function translate_vmovdqa_dqu_lsx)
dqa=$(extract_function translate_vmovdqa_lsx)
[[ -n "$lsx" && -n "$dqa" ]]
for required in \
    'ir1_opnd_is_ymm(dest) && ir1_opnd_is_mem(src)' \
    'ir1_opnd_is_mem(dest) && ir1_opnd_is_ymm(src)' \
    'ir1_opnd_is_ymm(dest) && ir1_opnd_is_ymm(src)' \
    'load_ymm_high128_shadow' \
    'store_ymm_high128_shadow' \
    'clear_ymm_high128_shadow' \
    'load_v128_from_ir1_mem_exact' \
    'store_v128_to_ir1_mem_exact' \
    'load_v256_from_ir1_mem_exact' \
    'store_v256_to_ir1_mem_exact' \
    'vmovaps_check_alignment'; do
    printf '%s\n' "$lsx" | grep -Fq "$required" || {
        echo "FAIL VMOVDQA LSX audit missing: $required" >&2
        exit 1
    }
done
grep -Fq 'check_guest_mem_range(address, 16, PAGE_READ)' "$exact"
grep -Fq 'check_guest_mem_range(address, 32, PAGE_READ)' "$exact"
grep -Fq 'gen_test_page_flag_force' "$exact"
if printf '%s\n' "$lsx" | grep -Eq '\bla_xv'; then
    echo 'FAIL VMOVDQA LSX function contains LASX generator' >&2
    exit 1
fi
if printf '%s\n' "$lsx" | grep -Fq 'option_enable_lasx'; then
    echo 'FAIL VMOVDQA LSX function selects backend internally' >&2
    exit 1
fi

grep -Fq 'translate_register_lsx(dt_X86_INS_VMOVDQA, translate_vmovdqa_lsx)' "$dispatch"
grep -Fq 'TRANS_FUNC_GEN(VMOVDQA, vmovdqa)' "$dispatch"

for required in \
    'vmovdqa xmm0, xmm0' 'vmovdqa ymm0, ymm0' \
    'xmm-load-u' 'ymm-store-u' 'xmm-load-cross' 'ymm-store-cross' \
    'xmm-load-page' 'ymm-store-page'; do
    grep -Fq "$required" "$fixture_s" "$fixture_c" || {
        echo "FAIL fixture missing: $required" >&2
        exit 1
    }
done

printf '%s\n' "$dqa" | grep -Fq 'translate_vmovdqa_dqu_lsx(pir1, true)'
if printf '%s\n' "$dqa" | grep -Eq '\bla_xv'; then
    echo 'FAIL VMOVDQA LSX wrapper contains LASX generator' >&2
    exit 1
fi

echo 'PASS WI-1896 VMOVDQA LSX source and x86 fixture audit'
