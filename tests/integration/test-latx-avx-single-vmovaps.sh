#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cases='xmm-load-u1 xmm-load-u8 xmm-load-u15 xmm-store-u1 xmm-store-u8 xmm-store-u15 ymm-load-u1 ymm-load-u16 ymm-load-u31 ymm-store-u1 ymm-store-u16 ymm-store-u31 xmm-load-cross xmm-store-cross ymm-load-cross ymm-store-cross xmm-load-noaccess xmm-store-noaccess ymm-load-noaccess ymm-store-noaccess'

usage()
{
    echo "usage: $0 generate PROBE OUTPUT_DIR" >&2
    echo "       $0 verify LATX_X86_64 PROBE EXPECTED_DIR SOURCE" >&2
    exit 2
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5
    set +e
    if [ -n "$runner" ]; then
        env LATX_AVX_CPUID=0 "$runner" "$probe" "$name" >"$output" 2>/dev/null
    else
        "$probe" "$name" >"$output" 2>/dev/null
    fi
    status=$?
    set -e
    printf '%s\n' "$status" >"$status_file"
}

check_probe()
{
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vmovaps
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2 expected=$3
    mkdir -p "$expected"
    check_probe "$probe"
    "$probe" >"$expected/normal.bin"
    [ "$(wc -c <"$expected/normal.bin")" -eq 560 ]
    for name in $cases; do
        run_case '' "$probe" "$name" "$expected/$name.bin" "$expected/$name.status"
        [ "$(sed -n '1p' "$expected/$name.status")" -eq 139 ]
        [ "$(wc -c <"$expected/$name.bin")" -eq 48 ]
    done
    echo "PASS generated x86 vmovaps reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2 probe=$3 expected=$4 source=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
    check_probe "$probe"
    env LATX_AVX_CPUID=0 "$latx" "$probe" >"$tmpdir/normal.bin"
    cmp "$expected/normal.bin" "$tmpdir/normal.bin"
    for name in $cases; do
        run_case "$latx" "$probe" "$name" "$tmpdir/$name.bin" "$tmpdir/$name.status"
        [ "$(sed -n '1p' "$tmpdir/$name.status")" -eq 139 ]
        [ "$(wc -c <"$tmpdir/$name.bin")" -eq 48 ]
        cmp "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
    done

    trace=$tmpdir/trace.log
    env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
        LATX_AVX_TRACE_YMM_INIT=1 "$latx" "$probe" trace >"$trace" 2>&1
    first=$(grep -F 'event=ymm_state' "$trace" | grep -F 'count=1 reg=15' | sed -n '1p')
    second=$(grep -F 'event=ymm_state' "$trace" | grep -F 'count=2 reg=15' | sed -n '1p')
    case "$first" in
        *'shadow_high0=1122334455667788 shadow_high1=99aabbccddeeff00'*) ;;
        *) echo "FAIL vmovaps initial YMM high" >&2; exit 1 ;;
    esac
    case "$second" in
        *'low0=8877665544332211 low1=0f1e2d3c4b5a6978'*\
*'shadow_high0=0000000000000000 shadow_high1=0000000000000000'*) ;;
        *) echo "FAIL vmovaps XMM high clear" >&2; exit 1 ;;
    esac

    body=$(awk '/bool translate_vmovaps\(/ { f=1 } f { print } f && /^}/ { exit }' "$source")
    printf '%s\n' "$body" | grep -Fq 'if (1)'
    printf '%s\n' "$body" | grep -Fq 'vmovaps_check_alignment'
    if printf '%s\n' "$body" | awk '/LSX-only path/ { f=1 } /Original LASX path/ { exit } f { print }' | grep -Eq '\bla_xv'; then
        echo "FAIL vmovaps LSX branch contains LASX" >&2
        exit 1
    fi
    dump=$(objdump -d --disassemble=translate_vmovaps "$latx")
    if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>'; then
        echo "FAIL compiled vmovaps calls LASX generator" >&2
        exit 1
    fi
    echo "PASS vmovaps x86/LATX differential, alignment faults and YMM state"
    ;;
*) usage ;;
esac
