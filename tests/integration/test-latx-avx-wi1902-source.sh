#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
source=$root/target/i386/latx/translator/tr-avx-mov.c
dispatch=$root/target/i386/latx/translator/translate.c
fixture_s=$root/tests/integration/latx-avx-single-vmovmsk.S
fixture_c=$root/tests/integration/latx-avx-single-vmovmsk.c
checker=$root/tests/integration/check-latx-avx-wi1902-mnemonic.sh

[[ -f "$source" && -f "$dispatch" && -f "$fixture_s" && \
   -f "$fixture_c" && -f "$checker" ]]
grep -Fq 'TRANS_FUNC_GEN(VMOVMSKPD, vmovmskpd)' "$dispatch"
grep -Fq 'TRANS_FUNC_GEN(VMOVMSKPS, vmovmskps)' "$dispatch"

grep -Fq 'translate_register_lsx(dt_X86_INS_VMOVMSKPD' "$dispatch" || {
    echo 'FAIL VMOVMSKPD LSX registration missing' >&2
    exit 1
}
grep -Fq 'translate_register_lsx(dt_X86_INS_VMOVMSKPS' "$dispatch" || {
    echo 'FAIL VMOVMSKPS LSX registration missing' >&2
    exit 1
}
grep -Fq 'bool translate_vmovmskpd_lsx' "$source" || {
    echo 'FAIL VMOVMSKPD LSX implementation missing' >&2
    exit 1
}
grep -Fq 'bool translate_vmovmskps_lsx' "$source" || {
    echo 'FAIL VMOVMSKPS LSX implementation missing' >&2
    exit 1
}
grep -Fq 'load_ymm_high128_shadow' "$source" || {
    echo 'FAIL VMOVMSK LSX implementation does not read YMM high-half shadow' >&2
    exit 1
}
grep -Fq 'store_ireg_to_ir1(result, dest_opnd, false)' "$source" || {
    echo 'FAIL VMOVMSK LSX implementation does not use width-aware GPR writeback' >&2
    exit 1
}
if sed -n '/static bool translate_vmovmsk_lsx/,/^bool translate_vmovmskpd_lsx/p' "$source" |
   grep -Fq 'la_xv'; then
    echo 'FAIL VMOVMSK LSX implementation emits LASX instruction' >&2
    exit 1
fi

for required in \
    'vmovmskps eax, xmm0' 'vmovmskps r9d, ymm0' \
    'vmovmskpd eax, xmm0' 'vmovmskpd r9d, ymm0' \
    'vmovdqu xmm15' 'vmovdqu ymm15' \
    '0x7fc00001' '0x7f800001' \
    '0x7ff8000000000001' '0x7ff0000000000001' \
    'lane == 0'; do
    grep -Fq "$required" "$fixture_s" "$fixture_c" || {
        echo "FAIL VMOVMSK fixture missing: $required" >&2
        exit 1
    }
done
for required in \
    'PS_XMM_CASES = 16' 'PS_YMM_CASES = 256' \
    'PD_XMM_CASES = 4' 'PD_YMM_CASES = 16' \
    'RECORD_SIZE = 16' 'OUTPUT_SIZE'; do
    grep -Fq "$required" "$fixture_c" || {
        echo "FAIL VMOVMSK fixture matrix missing: $required" >&2
        exit 1
    }
done

if grep -Eq 'vmovmsk(ps|pd).*ptr' "$fixture_s"; then
    echo 'FAIL VMOVMSK fixture contains invalid memory-source form' >&2
    exit 1
fi
echo 'PASS WI-1902 VMOVMSK LSX implementation and registration checks'
