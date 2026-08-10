#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
latx=${1:-$root/build64/latx-x86_64}
expected=${2:-/tmp/latx-avx-acceptance/fault-triad}
fixture_root=${3:-/tmp/latx-avx-acceptance}
work=$(mktemp -d /tmp/wi-1834-fault.XXXXXX)
trap 'rm -rf "$work"' EXIT HUP INT TERM

addr=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1 }')
[ -n "$addr" ]

run_lasx()
{
    name=$1
    probe=$2
    fault=$3
    set +e
    env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
        LATX_AVX_TRACE_YMM_INIT=1 "$latx" "$probe" "$fault" \
        >"$work/$name.lasx.bin" 2>"$work/$name.lasx.stderr"
    status=$?
    set -e
    printf '%s\n' "$status" >"$work/$name.lasx.status"
}

run_lsx()
{
    name=$1
    probe=$2
    fault=$3
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'handle SIGSEGV nostop noprint pass' \
        -ex 'handle SIGFPE nostop noprint pass' \
        -ex 'set environment LATX_AVX_CPUID 0' \
        -ex 'set environment LATX_AVX_TRACE 3' \
        -ex 'set environment LATX_AVX_TRACE_YMM 15' \
        -ex 'set environment LATX_AVX_TRACE_YMM_INIT 1' \
        -ex 'break translate_context_init' \
        -ex "run $probe $fault > $work/$name.lsx.bin 2> $work/$name.lsx.stderr" \
        -ex "set {int}$addr = 0" \
        -ex "x/wd $addr" \
        -ex continue >"$work/$name.lsx.gdb" 2>&1
    grep -Eq 'option_enable_lasx>:[[:space:]]+0' "$work/$name.lsx.gdb"
    grep -Fq 'exited with code 0213' "$work/$name.lsx.gdb"
}

check_source()
{
    grep -Fq 'gen_test_page_flag_force_range' \
        "$root/target/i386/latx/translator/tr-opnd-process.c"
    grep -Fq 'check_guest_mem_range(address, size, flag)' \
        "$root/target/i386/latx/translator/tr-opnd-process.c"
    grep -Fq 'load_u64_from_ir1_mem_exact(opnd2)' \
        "$root/target/i386/latx/translator/tr-avx-cvt.c"
}

check_case()
{
    name=$1
    mode=$2
    expected_size=$3
    expected_name=$4
    cmp -n 32 "$expected/$expected_name.x86.bin" "$work/$name.lasx.bin"
    cmp -n 32 "$expected/$expected_name.x86.bin" "$work/$name.lsx.bin"
    [ "$(cat "$expected/$expected_name.x86.status")" = 139 ]
    [ "$(cat "$work/$name.lasx.status")" = 139 ]
    grep -Fq 'exited with code 0213' "$work/$name.lsx.gdb"
    [ "$(wc -c <"$work/$name.lasx.bin")" = "$expected_size" ]
    [ "$(wc -c <"$work/$name.lsx.bin")" = "$expected_size" ]
    if [ "$mode" = exact ]; then
        cmp "$expected/$expected_name.x86.bin" "$work/$name.lasx.bin"
        cmp "$expected/$expected_name.x86.bin" "$work/$name.lsx.bin"
    fi
}

check_source
run_lasx vpand "$fixture_root/latx-avx-single-vpand.static" xmm-cross-1
run_lsx vpand "$fixture_root/latx-avx-single-vpand.static" xmm-cross-1
run_lasx vmovups "$fixture_root/latx-avx-single-vmovups.static" xmm-store-cross-1
run_lsx vmovups "$fixture_root/latx-avx-single-vmovups.static" xmm-store-cross-1
run_lasx vcvtsi2sd "$fixture_root/latx-avx-single-vcvtsi2sd.static" fault-m64
run_lsx vcvtsi2sd "$fixture_root/latx-avx-single-vcvtsi2sd.static" fault-m64

check_case vpand prefix 48 vpand.xmm-cross-1
check_case vmovups exact 80 vmovups.xmm-store-cross-1
check_case vcvtsi2sd exact 64 vcvtsi2sd.fault-m64
echo 'PASS WI-1834 LASX precise memory fault helper representatives'
