#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 LATX_X86_64 STATIC_X86_PROBE" >&2
    exit 2
fi

latx=$1
probe=$2
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

if [ ! -x "$latx" ] || [ ! -x "$probe" ]; then
    echo "FAIL LATX and probe must be executable" >&2
    exit 2
fi

"$(dirname "$0")/check-latx-avx-single-mnemonic.sh" "$probe" vmovq

check_register()
{
    reg=$1
    value=$2
    output=$tmpdir/xmm$reg.log

    env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 \
        LATX_AVX_TRACE_YMM="$reg" LATX_AVX_TRACE_YMM_INIT=1 \
        "$latx" "$probe" "$reg" >"$output" 2>&1

    if ! grep -Fq 'event=ymm_init' "$output" ||
       ! grep -Fq 'shadow_high0=1122334455667788 shadow_high1=99aabbccddeeff00' \
           "$output"; then
        echo "FAIL xmm$reg YMM high half was not initialized" >&2
        sed -n '1,120p' "$output" >&2
        exit 1
    fi
    second=$(grep -F 'event=ymm_state' "$output" | sed -n '2p')
    case "$second" in
        *"count=2 reg=$reg low0=$value low1=0000000000000000"*\
*'shadow_high0=0000000000000000 shadow_high1=0000000000000000'*) ;;
        *)
            echo "FAIL xmm$reg vmovq state after first execution" >&2
            sed -n '1,120p' "$output" >&2
            exit 1
            ;;
    esac
}

check_register 0 8877665544332211
check_register 15 0f1e2d3c4b5a6978
echo "PASS vmovq clears XMM upper 64 and YMM shadow high 128 for XMM0/XMM15"
