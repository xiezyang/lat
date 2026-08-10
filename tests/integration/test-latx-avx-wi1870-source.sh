#!/usr/bin/env bash
set -euo pipefail

repo=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
cmp_source="$repo/target/i386/latx/translator/tr-avx-cmp.c"
dispatch="$repo/target/i386/latx/translator/translate.c"
[[ -f "$cmp_source" && -f "$dispatch" ]] || { echo "FAIL source missing" >&2; exit 2; }

grep -Fq 'bool translate_vcomisd(IR1_INST * pir1)' "$cmp_source"
grep -Fq 'bool translate_vcomiss(IR1_INST * pir1)' "$cmp_source"
grep -Fq 'bool translate_vucomiss(IR1_INST * pir1)' "$cmp_source"
grep -Eq 'bool translate_vucomisd_lsx\(IR1_INST \* ?pir1\)' "$cmp_source"
grep -Fq 'TRANS_FUNC_GEN(VCOMISD, vcomisd)' "$dispatch"
grep -Fq 'TRANS_FUNC_GEN(VCOMISS, vcomiss)' "$dispatch"
grep -Fq 'TRANS_FUNC_GEN(VUCOMISS, vucomiss)' "$dispatch"
grep -Fq 'translate_register_lsx(dt_X86_INS_VUCOMISD, translate_vucomisd_lsx)' "$dispatch"

for function in translate_vcomisd_lsx translate_vcomiss_lsx translate_vucomiss_lsx; do
    grep -Eq "bool $function\\(IR1_INST \\* ?pir1\\)" "$cmp_source"
done
for entry in VCOMISD VCOMISS VUCOMISS; do
    grep -Fq "translate_register_lsx(dt_X86_INS_$entry" "$dispatch"
done

for instruction in vcomisd vcomiss vucomiss; do
    grep -Fq "\"$instruction\"" "$repo/tests/integration/wi1870-comis-fixtures.json"
done
echo 'PASS WI-1870 COMIS source and fixture manifest audit'
