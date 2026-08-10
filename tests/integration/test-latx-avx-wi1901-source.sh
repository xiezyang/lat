#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
source=$root/target/i386/latx/translator/tr-avx-mov.c
dispatch=$root/target/i386/latx/translator/translate.c
fixture_s=$root/tests/integration/latx-avx-single-vmovddup.S
fixture_c=$root/tests/integration/latx-avx-single-vmovddup.c
runner=$root/tests/integration/test-latx-avx-single-vmovddup.sh

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

[[ -f "$source" && -f "$dispatch" && -f "$fixture_s" && \
   -f "$fixture_c" && -f "$runner" ]]
lsx=$(extract_function translate_vmovddup_lsx)
lasx=$(extract_function translate_vmovddup)
[[ -n "$lsx" && -n "$lasx" ]]
for required in \
    'load_u64_from_ir1_mem_exact' \
    'load_v256_from_ir1_mem_exact' \
    'la_vreplgr2vr_d' \
    'la_vreplvei_d' \
    'clear_ymm_high128_shadow' \
    'store_ymm_high128_shadow'; do
    printf '%s\n' "$lsx" | grep -Fq "$required" || {
        echo "FAIL VMOVDDUP LSX audit missing: $required" >&2
        exit 1
    }
done
if printf '%s\n' "$lsx" | grep -Eq '\bla_xv|option_enable_lasx'; then
    echo 'FAIL VMOVDDUP LSX function contains LASX generator or backend selection' >&2
    exit 1
fi
printf '%s\n' "$lasx" | grep -Eq '\bla_xv'
grep -Fq 'translate_register_lsx(dt_X86_INS_VMOVDDUP, translate_vmovddup_lsx)' "$dispatch"
grep -Fq 'TRANS_FUNC_GEN(VMOVDDUP, vmovddup)' "$dispatch"

for required in \
    'vmovddup xmm0' 'vmovddup ymm0' 'vmovddup xmm15' 'vmovddup ymm15' \
    '0x7ff8000000000001' '0x7ff0000000000001' \
    '4088' '4092' '4064' '4080'; do
    grep -Fq "$required" "$fixture_s" "$fixture_c" || {
        echo "FAIL VMOVDDUP fixture missing: $required" >&2
        exit 1
    }
done
for required in \
    'm64-cross-1' 'm64-cross-7' 'ymm-cross-1' 'ymm-cross-31' \
    'check_fault_record' 'ymmh15'; do
    grep -Fq "$required" "$runner" "$fixture_c" || {
        echo "FAIL VMOVDDUP fault/high-half coverage missing: $required" >&2
        exit 1
    }
done

echo 'PASS WI-1901 VMOVDDUP LSX dispatch, LASX preservation and fixture audit'
