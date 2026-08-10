#!/usr/bin/env bash
set -euo pipefail

repo=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
cvt="$repo/target/i386/latx/translator/tr-avx-cvt.c"
dispatch="$repo/target/i386/latx/translator/translate.c"
[[ -f "$cvt" && -f "$dispatch" ]] || { echo "FAIL source files missing" >&2; exit 2; }

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
    ' "$cvt"
}

lsx=$(extract_function translate_vcvtsd2ss_lsx)
lasx=$(extract_function translate_vcvtsd2ss)
printf '%s\n' "$lsx" | grep -Fq 'la_fcvt_s_d'
printf '%s\n' "$lsx" | grep -Fq 'load_freg128_from_ir1'
printf '%s\n' "$lsx" | grep -Fq 'clear_ymm_high128_shadow'
printf '%s\n' "$lsx" | grep -Fq 'tr_save_ymm_to_env(UINT16_MAX)'
if printf '%s\n' "$lsx" | grep -Eq '\bla_xv'; then
    echo 'FAIL VCVTSD2SS LSX function contains LASX generator' >&2
    exit 1
fi
if printf '%s\n' "$lsx" | grep -Fq 'option_enable_lasx'; then
    echo 'FAIL VCVTSD2SS LSX function selects backend internally' >&2
    exit 1
fi
printf '%s\n' "$lasx" | grep -Fq 'la_xvori_b'
grep -Fq 'translate_register_lsx(dt_X86_INS_VCVTSD2SS, translate_vcvtsd2ss_lsx)' "$dispatch"
grep -Fq 'tr_save_ymm_to_env(UINT16_MAX)' "$cvt"
grep -Fq 'fpe-overflow' "$repo/tests/integration/latx-avx-single-vcvtsd2ss.c"
grep -Fq 'fpe-underflow' "$repo/tests/integration/latx-avx-single-vcvtsd2ss.c"

echo 'PASS WI-1875 VCVTSD2SS LSX source audit'
