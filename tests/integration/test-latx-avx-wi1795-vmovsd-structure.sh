#!/usr/bin/env bash

set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
probe=${2:-}
binary=${3:-}
source="$root/target/i386/latx/translator/tr-avx-mov.c"
header="$root/target/i386/latx/include/translate.h"
dispatch="$root/target/i386/latx/translator/translate.c"

extract_function()
{
    local symbol=$1
    awk -v symbol="$symbol" '
            index($0, symbol) > 0 {
                capture = 1
                depth = 0
                opened = 0
                current = ""
            }
        capture {
            current = current $0 "\n"
                opens = gsub(/\{/, "{")
                closes = gsub(/\}/, "}")
                depth += opens - closes
                if (opens > 0) {
                    opened = 1
                }
                if (opened && depth == 0) {
                    latest = current
                    capture = 0
                }
        }
        END { printf "%s", latest }
    ' "$source"
}

lsx=$(extract_function 'bool translate_vmovsd_lsx(')
lasx=$(extract_function 'bool translate_vmovsd(')

[ -n "$lsx" ] || { echo "FAIL missing translate_vmovsd_lsx" >&2; exit 1; }
[ -n "$lasx" ] || { echo "FAIL missing translate_vmovsd" >&2; exit 1; }

if printf '%s\n' "$lsx" |
    grep -Eq '\bla_xv|option_enable_lasx|if[[:space:]]*\([[:space:]]*1[[:space:]]*\)|translate_vmovsd\('; then
    echo "FAIL VMOVSD LSX function contains LASX or backend selection" >&2
    exit 1
fi
if printf '%s\n' "$lasx" |
    grep -Eq 'option_enable_lasx|if[[:space:]]*\([[:space:]]*1[[:space:]]*\)|translate_vmovsd_lsx'; then
    echo "FAIL VMOVSD LASX function contains backend selection" >&2
    exit 1
fi

for required in load_u64_from_ir1_mem_exact store_u64_to_ir1_mem_exact \
                la_vinsgr2vr_d la_vpickve2gr_du clear_ymm_high128_shadow; do
    printf '%s\n' "$lsx" | grep -Fq "$required" || {
        echo "FAIL VMOVSD LSX function misses $required" >&2
        exit 1
    }
done
printf '%s\n' "$lasx" | grep -Eq '\bla_xv' || {
    echo "FAIL VMOVSD LASX function has no LASX generator" >&2
    exit 1
}

grep -Fq 'TRANS_FUNC_DEF(vmovsd_lsx)' "$header"
grep -Fq 'translate_register_lsx(dt_X86_INS_VMOVSD, translate_vmovsd_lsx)' \
    "$dispatch"
echo "PASS VMOVSD split, declaration and LSX registration source checks"

if [ -n "$probe" ]; then
    "$root/tests/integration/check-latx-avx-single-mnemonic.sh" "$probe" vmovsd
fi

if [ -n "$binary" ]; then
    dump=$(objdump -d --disassemble=translate_vmovsd_lsx "$binary")
    if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>'; then
        echo "FAIL compiled translate_vmovsd_lsx calls LASX generator" >&2
        exit 1
    fi
    echo "PASS compiled translate_vmovsd_lsx has no LASX generator call"
fi
