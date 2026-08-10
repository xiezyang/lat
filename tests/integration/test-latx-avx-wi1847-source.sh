#!/usr/bin/env bash
set -euo pipefail

source=target/i386/latx/translator/translate.c

grep -Fq 'bool avx_isa;' "$source"
grep -Fq '{ function, LATX_AVX_OPT_ENTRY, LATX_AVX_OPT_ENTRY }' "$source"

gate=$(sed -n '/if (!option_avx_cpuid && translate_functions\[tr_func_idx\].avx_isa)/,/return translate_invalid(ir1);/p' "$source")
grep -Fq 'translate_invalid(ir1);' <<<"$gate"
if grep -Fq 'opcode[0]' <<<"$gate" || grep -Fq 'avx_opt_only' <<<"$gate" || \
    grep -Fq 'isa_features' <<<"$gate"; then
    echo 'FAIL hidden AVX gate uses a non-ISA attribute' >&2
    exit 1
fi

if sed -n '/static void translate_register_lsx/,/^}/p' "$source" | \
    grep -Fq 'avx_isa'; then
    echo 'FAIL LSX registration can clear avx_isa' >&2
    exit 1
fi

awk '
/^#define LATX_AVX_OPT_ENTRY true/ { state = "true" }
/^#define LATX_AVX_OPT_ENTRY false/ { state = "false" }
/TRANS_FUNC_GEN\(PEXT,/ { pext = state }
/TRANS_FUNC_GEN\(PDEP,/ { pdep = state }
END {
    if (pext != "false" || pdep != "false") {
        printf "FAIL PEXT/PDEP avx_isa state: PEXT=%s PDEP=%s\n", pext, pdep
        exit 1
    }
}' "$source"

if grep -q '\[WI1847_CLASS\]' "$source"; then
    echo 'FAIL temporary classification diagnostic remains' >&2
    exit 1
fi

echo 'PASS AVX ISA attribute survives LSX registration and excludes BMI2'
