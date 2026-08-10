#!/bin/sh
set -eu

if [ "$#" -ne 4 ]; then
    echo "usage: $0 LATX_X86_64 INFO_PROBE VEX_PROBE BMI2_PROBE" >&2
    exit 2
fi

latx=$1
info_probe=$2
vex_probe=$3
bmi2_probe=$4
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

info=$(env LATX_AVX_CPUID=0 "$latx" "$info_probe" info)
printf '%s\n' "$info" | grep -Fq 'fma=0 xsave=0 osxsave=0 avx=0 f16c=0'
printf '%s\n' "$info" | grep -Fq 'bmi1=0 hle=0 avx2=0 bmi2=0 avx512f=0'
printf '%s\n' "$info" | grep -Fq \
    'leafd0_eax=0x00000000 leafd0_ebx=0x00000000 leafd0_ecx=0x00000000 leafd0_edx=0x00000000'
echo "PASS hidden CPUID feature bits"

set +e
env LATX_AVX_CPUID=0 "$latx" "$vex_probe" \
    >"$tmpdir/hidden-vex.out" 2>"$tmpdir/hidden-vex.err"
hidden_status=$?
env LATX_AVX_CPUID=1 "$latx" "$vex_probe" \
    >"$tmpdir/enabled-vex.out" 2>"$tmpdir/enabled-vex.err"
enabled_status=$?
env LATX_AVX_CPUID=0 "$latx" "$bmi2_probe" \
    >"$tmpdir/hidden-bmi2.out" 2>"$tmpdir/hidden-bmi2.err"
bmi2_status=$?
set -e

[ "$hidden_status" -eq 132 ]
[ "$enabled_status" -eq 0 ]
[ "$bmi2_status" -eq 0 ]
echo "PASS hidden VEX #UD status=132"
echo "PASS enabled VEX execution status=0"
echo "PASS hidden BMI2 control status=0"

printf 'hidden_vex_status=%s\n' "$hidden_status"
printf 'enabled_vex_status=%s\n' "$enabled_status"
printf 'hidden_bmi2_status=%s\n' "$bmi2_status"
