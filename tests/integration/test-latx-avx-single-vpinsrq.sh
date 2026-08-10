#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

usage()
{
    echo "usage: $0 generate PROBE OUTPUT_DIR" >&2
    echo "       $0 verify LATX_X86_64 PROBE EXPECTED_DIR TR_AVX_SOURCE" >&2
    exit 2
}

run_case()
{
    runner=$1
    probe=$2
    name=$3
    output=$4
    status_file=$5

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

check_fault_record()
{
    name=$1
    output=$2
    status_file=$3
    record_hex=$(od -An -tx1 "$output" | tr -d ' \n')

    if [ "$(sed -n '1p' "$status_file")" != "139" ]; then
        echo "FAIL $name status: $(sed -n '1p' "$status_file")" >&2
        exit 1
    fi
    if [ "$(wc -c <"$output")" -ne 32 ]; then
        echo "FAIL $name fault record size: $(wc -c <"$output")" >&2
        exit 1
    fi
    case "$name:$record_hex" in
        fault-invalid:0b000000010000000100000000000000275e9dc08ab8136f50f8d6317c09a2e4) ;;
        fault-cross-page:0b000000020000000010000000000000275e9dc08ab8136f50f8d6317c09a2e4) ;;
        *)
            echo "FAIL $name signal or preserved XMM15: $record_hex" >&2
            exit 1
            ;;
    esac
}

check_probe()
{
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vpinsrq
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2
    output_dir=$3
    mkdir -p "$output_dir"
    check_probe "$probe"
    "$probe" >"$output_dir/normal.bin"
    if [ "$(wc -c <"$output_dir/normal.bin")" -ne 8384 ]; then
        echo "FAIL normal output size" >&2
        exit 1
    fi
    run_case "" "$probe" fault-invalid \
        "$output_dir/fault-invalid.bin" "$output_dir/fault-invalid.status"
    run_case "" "$probe" fault-cross-page \
        "$output_dir/fault-cross-page.bin" "$output_dir/fault-cross-page.status"
    check_fault_record fault-invalid "$output_dir/fault-invalid.bin" \
        "$output_dir/fault-invalid.status"
    check_fault_record fault-cross-page "$output_dir/fault-cross-page.bin" \
        "$output_dir/fault-cross-page.status"
    echo "PASS generated x86 vpinsrq reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2
    probe=$3
    expected_dir=$4
    source_file=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

    check_probe "$probe"
    env LATX_AVX_CPUID=0 "$latx" "$probe" >"$tmpdir/normal.bin"
    cmp "$expected_dir/normal.bin" "$tmpdir/normal.bin"

    for name in fault-invalid fault-cross-page; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status"
        check_fault_record "$name" "$tmpdir/$name.bin" "$tmpdir/$name.status"
        cmp "$expected_dir/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected_dir/$name.status" "$tmpdir/$name.status"
    done

    trace=$tmpdir/trace.log
    env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
        LATX_AVX_TRACE_YMM_INIT=1 "$latx" "$probe" trace >"$trace" 2>&1
    grep -Fq 'event=ymm_init' "$trace"
    first=$(grep -F 'event=ymm_state' "$trace" |
        grep -F 'count=1 reg=15' | sed -n '1p')
    second=$(grep -F 'event=ymm_state' "$trace" |
        grep -F 'count=2 reg=15' | sed -n '1p')
    case "$first" in
        *'shadow_high0=1122334455667788 shadow_high1=99aabbccddeeff00'*) ;;
        *) echo "FAIL vpinsrq nonzero initial high half" >&2; exit 1 ;;
    esac
    case "$second" in
        *'shadow_high0=0000000000000000 shadow_high1=0000000000000000'*) ;;
        *) echo "FAIL vpinsrq did not clear high half" >&2; exit 1 ;;
    esac

    lsx_branch=$(awk '
        /bool translate_vpinsrq\(/ { in_function = 1 }
        in_function && /if \(1\)/ { in_branch = 1 }
        in_branch && /} else {/ { exit }
        in_branch { print }
    ' "$source_file")
    printf '%s\n' "$lsx_branch" | grep -Fq 'clear_ymm_high128_shadow'
    if printf '%s\n' "$lsx_branch" | grep -Eq 'la_xv|set_high128_xreg_to_zero'; then
        echo "FAIL vpinsrq LSX branch contains LASX high-half handling" >&2
        exit 1
    fi
    grep -Fq 'TRANS_FUNC_GEN(VPINSRQ, vpinsrq)' \
        "$(dirname "$source_file")/translate.c"

    echo "PASS vpinsrq x86/LATX CPUID0 differential and YMM clearing"
    ;;
*)
    usage
    ;;
esac
