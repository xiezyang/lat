#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 AVX_LATX NO_AVX_LATX STATIC_X86_PROBE" >&2
    exit 2
fi

avx_latx=$1
no_avx_latx=$2
probe=$3

assert_contains() {
    output=$1
    expected=$2
    label=$3

    if ! printf '%s\n' "$output" | grep -Fq "$expected"; then
        echo "FAIL $label: missing '$expected'" >&2
        printf '%s\n' "$output" >&2
        exit 1
    fi
    echo "PASS $label"
}

avx_config=$(dirname "$avx_latx")/config-host.mak
no_avx_config=$(dirname "$no_avx_latx")/config-host.mak

assert_contains "$(sed -n '/^CONFIG_LATX_AVX_OPT=/p' "$avx_config")" \
    'CONFIG_LATX_AVX_OPT=y' avx_build_enabled
if grep -Fq 'CONFIG_LATX_AVX_OPT=y' "$no_avx_config"; then
    echo "FAIL no_avx_build_disabled: CONFIG_LATX_AVX_OPT remains enabled" >&2
    exit 1
fi
echo "PASS no_avx_build_disabled"

no_avx_bmi2=$($no_avx_latx "$probe" bmi2)
no_avx_vector=$($no_avx_latx "$probe" unconditional)
avx_vector=$(env LATX_AVX_CPUID=0 "$avx_latx" "$probe" unconditional)

assert_contains "$no_avx_bmi2" 'bmi2=executed result=7' \
    bmi2_translates_without_a
if printf '%s\n' "$no_avx_vector" |
    grep -Fq 'unconditional_avx=executed result=2'; then
    echo "FAIL avx_requires_a: no-AVX build returned the AVX result" >&2
    exit 1
fi
echo "PASS avx_requires_a"
assert_contains "$avx_vector" 'unconditional_avx=executed result=2' \
    avx_translates_with_a

printf '%s\n' "$no_avx_bmi2" | grep 'bmi2='
printf '%s\n' "$no_avx_vector" | grep 'unconditional_avx='
printf '%s\n' "$avx_vector" | grep 'unconditional_avx='
