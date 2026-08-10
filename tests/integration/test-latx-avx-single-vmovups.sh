#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cases='xmm-load-cross-1 xmm-load-cross-15 xmm-store-cross-1 xmm-store-cross-15 ymm-load-cross-1 ymm-load-cross-15 ymm-load-cross-16 ymm-load-cross-31 ymm-store-cross-1 ymm-store-cross-15 ymm-store-cross-16 ymm-store-cross-31'

usage()
{
    echo "usage: $0 generate PROBE OUTPUT_DIR" >&2
    echo "       $0 verify LATX_X86_64 PROBE EXPECTED_DIR HELPER_SOURCE VMOVUPS_SOURCE" >&2
    exit 2
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5
    set +e
    if [ -n "$runner" ]; then
        env LATX_AVX_CPUID="${LATX_AVX_CPUID_VALUE:-0}" \
            "$runner" "$probe" "$name" >"$output" 2>/dev/null
    else
        "$probe" "$name" >"$output" 2>/dev/null
    fi
    status=$?
    set -e
    printf '%s\n' "$status" >"$status_file"
}

check_probe()
{
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vmovups
}

normalise_register_reference()
{
    od -An -v -tx8 -w32 "$1" | awk '{ print $1, $2, $3, $4 }'
}

normalise_register_trace()
{
    awk '
        /event=hit/ {
            observer = index($0, "bytes=c4417c10f6") != 0
            next
        }
        observer && /event=ymm_state/ {
            for (i = 1; i <= NF; ++i) {
                split($i, field, "=")
                if (field[1] == "low0") low0 = field[2]
                if (field[1] == "low1") low1 = field[2]
                if (field[1] == "shadow_high0") high0 = field[2]
                if (field[1] == "shadow_high1") high1 = field[2]
            }
            print low0, low1, high0, high1
            observer = 0
        }
    ' "$1"
}

merge_register_traces()
{
    trace15=$1 trace0=$2
    awk '
        NR == FNR { reg15[NR] = $0; next }
        {
            line_no = FNR
            use15 = ((line_no - 1) % 2) == 0
            print use15 ? reg15[line_no] : $0
        }
    ' "$trace15" "$trace0"
}

check_fault_record()
{
    record=$1
    signal_number=$(od -An -td4 -N4 "$record")
    signal_code=$(od -An -td4 -j4 -N4 "$record")
    fault_offset=$(od -An -tu8 -j8 -N8 "$record")

    [ "$signal_number" -eq 11 ]
    [ "$signal_code" -eq 2 ]
    [ "$fault_offset" -eq 4096 ]
}

check_stores_unchanged()
{
    directory=$1

    cmp "$directory/xmm-load-cross-1.bin" \
        "$directory/xmm-store-cross-1.bin"
    cmp "$directory/xmm-load-cross-15.bin" \
        "$directory/xmm-store-cross-15.bin"
    for count in 1 15 16 31; do
        cmp "$directory/ymm-load-cross-$count.bin" \
            "$directory/ymm-store-cross-$count.bin"
    done
}

check_sources()
{
    helper_source=$1 vmovups_source=$2

    grep -Fq 'IR2_OPND load_v128_from_ir1_mem_exact' "$helper_source"
    grep -Fq 'void store_v128_to_ir1_mem_exact' "$helper_source"
    grep -Fq 'void load_v256_from_ir1_mem_exact' "$helper_source"
    grep -Fq 'void store_v256_to_ir1_mem_exact' "$helper_source"
    grep -Fq 'check_guest_mem_range(address, 16, PAGE_READ)' "$helper_source"
    grep -Fq 'check_guest_mem_range(address, 32, PAGE_READ)' "$helper_source"
    grep -Fq 'check_guest_mem_range(address, 16, PAGE_WRITE | PAGE_WRITE_ORG)' "$helper_source"
    grep -Fq 'check_guest_mem_range(address, 32, PAGE_WRITE | PAGE_WRITE_ORG)' "$helper_source"

    body=$(awk '/bool translate_vmovups\(/ { f=1 } f { print } f && /^}/ { exit }' "$vmovups_source")
    printf '%s\n' "$body" | grep -Fq 'if (1)'
    printf '%s\n' "$body" | grep -Fq 'load_v128_from_ir1_mem_exact'
    printf '%s\n' "$body" | grep -Fq 'store_v128_to_ir1_mem_exact'
    printf '%s\n' "$body" | grep -Fq 'load_v256_from_ir1_mem_exact'
    printf '%s\n' "$body" | grep -Fq 'store_v256_to_ir1_mem_exact'
    if printf '%s\n' "$body" |
        awk '/LSX-only path/ { f=1 } /Original LASX path/ { exit } f { print }' |
        grep -Eq '\bla_xv'; then
        echo "FAIL vmovups LSX branch contains LASX" >&2
        exit 1
    fi
}

check_host_code()
{
    latx=$1
    for symbol in load_v128_from_ir1_mem_exact store_v128_to_ir1_mem_exact \
        load_v256_from_ir1_mem_exact store_v256_to_ir1_mem_exact \
        translate_vmovups; do
        dump=$(objdump -d --disassemble="$symbol" "$latx")
        if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>'; then
            echo "FAIL compiled $symbol calls LASX generator" >&2
            exit 1
        fi
    done
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2 expected=$3
    mkdir -p "$expected"
    check_probe "$probe"
    "$probe" >"$expected/normal.bin"
    [ "$(wc -c <"$expected/normal.bin")" -eq 4544 ]
    "$probe" register-reference >"$expected/register-reference.bin"
    [ "$(wc -c <"$expected/register-reference.bin")" -eq 384 ]
    normalise_register_reference "$expected/register-reference.bin" \
        >"$expected/register-state.txt"
    [ "$(wc -l <"$expected/register-state.txt")" -eq 12 ]
    for name in $cases; do
        run_case '' "$probe" "$name" "$expected/$name.bin" "$expected/$name.status"
        [ "$(sed -n '1p' "$expected/$name.status")" -eq 139 ]
        [ "$(wc -c <"$expected/$name.bin")" -eq 80 ]
        check_fault_record "$expected/$name.bin"
    done
    check_stores_unchanged "$expected"
    echo "PASS generated x86 vmovups reference"
    ;;
verify)
    [ "$#" -eq 6 ] || usage
    latx=$2 probe=$3 expected=$4 helper_source=$5 vmovups_source=$6
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
    check_probe "$probe"
    env LATX_AVX_CPUID="${LATX_AVX_CPUID_VALUE:-0}" \
        "$latx" "$probe" >"$tmpdir/normal.bin"
    [ "$(wc -c <"$tmpdir/normal.bin")" -eq 4544 ]
    cmp "$expected/normal.bin" "$tmpdir/normal.bin"
    for reg in 15 0; do
        env LATX_AVX_CPUID="${LATX_AVX_CPUID_VALUE:-0}" LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM="$reg" \
            "$latx" "$probe" register-trace \
            >"$tmpdir/register-$reg.stdout" \
            2>"$tmpdir/register-$reg.log"
        normalise_register_trace "$tmpdir/register-$reg.log" \
            >"$tmpdir/register-state-$reg.txt"
        [ "$(wc -l <"$tmpdir/register-state-$reg.txt")" -eq 12 ]
    done
    merge_register_traces "$tmpdir/register-state-15.txt" \
        "$tmpdir/register-state-0.txt" >"$tmpdir/register-state.txt"
    cmp "$expected/register-state.txt" "$tmpdir/register-state.txt"
    for name in $cases; do
        run_case "$latx" "$probe" "$name" "$tmpdir/$name.bin" "$tmpdir/$name.status"
        [ "$(sed -n '1p' "$tmpdir/$name.status")" -eq 139 ]
        [ "$(wc -c <"$tmpdir/$name.bin")" -eq 80 ]
        cmp "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
        check_fault_record "$tmpdir/$name.bin"
    done
    check_stores_unchanged "$tmpdir"
    check_sources "$helper_source" "$vmovups_source"
    check_host_code "$latx"
    echo "PASS vmovups all forms, YMM state and exact cross-page differential"
    ;;
*) usage ;;
esac
