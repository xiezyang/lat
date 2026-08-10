#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
fault_cases='xmm-cross-1 xmm-cross-15 ymm-cross-1 ymm-cross-15'
reference_bin_sha256=61ff3b4db0511380f1818be451dd936d220c7ffee4b8556da769f585bb3fc0e0
reference_state_sha256=a2b68b21d1fe0b3beb50f413eddf7ebb9afa104bbeb521543cafe1d6ef80090d

usage()
{
    echo "usage: $0 generate PROBE OUTPUT_DIR" >&2
    echo "       $0 verify LATX_X86_64 PROBE EXPECTED_DIR SOURCE" >&2
    exit 2
}

check_probe()
{
    if [ "${LATX_AVX_SKIP_PROBE_CHECK:-0}" = 1 ]; then
        echo "SKIP single AVX mnemonic check: x86 objdump unavailable"
        return
    fi
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vpsrlq
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5 error=$6
    set +e
    if [ -n "$runner" ]; then
        env LATX_AVX_CPUID=1 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
            LATX_AVX_TRACE_YMM_INIT=1 \
            "$runner" "$probe" "$name" >"$output" 2>"$error"
    else
        "$probe" "$name" >"$output" 2>"$error"
    fi
    status=$?
    set -e
    printf '%s\n' "$status" >"$status_file"
}

normalise_reference()
{
    od -An -v -tx8 -w32 "$1" | awk '{ print $1, $2, $3, $4 }'
}

check_reference_provenance()
{
    reference_dir=$1
    actual_bin=$(sha256sum "$reference_dir/reference.bin" | awk '{print $1}')
    actual_state=$(sha256sum "$reference_dir/state.txt" | awk '{print $1}')
    printf 'reference.bin sha256=%s\n' "$actual_bin"
    printf 'state.txt sha256=%s\n' "$actual_state"
    [ "$actual_bin" = "$reference_bin_sha256" ] || {
        echo "FAIL trusted reference.bin hash" >&2
        exit 1
    }
    [ "$actual_state" = "$reference_state_sha256" ] || {
        echo "FAIL trusted state.txt hash" >&2
        exit 1
    }
}

run_lsx_case()
{
    latx=$1 probe=$2 argument=$3 output=$4 error=$5 gdb_log=$6
    option_address=$(nm -g "$latx" | awk '$3 == "option_enable_lasx" {
        print "0x" $1
    }')
    [ -n "$option_address" ] || {
        echo "FAIL option_enable_lasx symbol is missing" >&2
        exit 1
    }
    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AVX_TRACE 3' \
        -ex 'set environment LATX_AVX_TRACE_YMM 15' \
        -ex 'set environment LATX_AVX_TRACE_YMM_INIT 0' \
        -ex 'break translate_context_init' \
        -ex "run $probe $argument > $output 2> $error" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue >"$gdb_log" 2>&1
    gdb_status=$?
    set -e
    [ "$gdb_status" -eq 0 ] || {
        echo "FAIL gdb LSX runner: $gdb_status" >&2
        cat "$gdb_log" >&2
        exit 1
    }
    grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$gdb_log" || {
        echo "FAIL LSX runner did not set option_enable_lasx=0" >&2
        cat "$gdb_log" >&2
        exit 1
    }
    if grep -q 'exited with code 0213' "$gdb_log"; then
        printf '139\n' >"$output.status"
    elif grep -q 'exited normally' "$gdb_log"; then
        printf '0\n' >"$output.status"
    else
        printf '1\n' >"$output.status"
    fi
}

run_lasx_case()
{
    latx=$1 probe=$2 argument=$3 output=$4 error=$5
    set +e
    env LATX_AVX_CPUID=1 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
        LATX_AVX_TRACE_YMM_INIT=0 \
        "$latx" "$probe" "$argument" >"$output" 2>"$error"
    runner_status=$?
    set -e
    printf '%s\n' "$runner_status" >"$output.status"
}

check_guest_reference()
{
    reference_file=$1 actual_file=$2 mode=$3
    cmp "$reference_file" "$actual_file" || {
        echo "FAIL $mode guest reference stdout differs from x86" >&2
        cmp -l "$reference_file" "$actual_file" | head -n 16 >&2 || true
        exit 1
    }
    printf 'PASS %s guest reference stdout matches x86 (%s bytes)\n' \
        "$mode" "$(wc -c <"$actual_file")"
}

check_fault_case()
{
    expected_file=$1 actual_file=$2 status_file=$3 mode=$4
    check_fault_record "$actual_file" "$status_file"
    cmp -n 32 "$expected_file" "$actual_file"
    printf 'PASS %s %s fault record guest prefix\n' "$mode" "$(basename "$actual_file")"
}

check_fault_record()
{
    output=$1 status_file=$2
    if [ "$(sed -n '1p' "$status_file")" != 139 ]; then
        echo "FAIL fault status: $(sed -n '1p' "$status_file")" >&2
        exit 1
    fi
    if [ "$(wc -c <"$output")" -ne 48 ]; then
        echo "FAIL fault record size: $(wc -c <"$output")" >&2
        exit 1
    fi
    header=$(od -An -v -tx4 -N8 "$output" | tr -d ' \n')
    case "$header" in
        0000000b00000002) ;;
        *) echo "FAIL fault signal/code: $header" >&2; exit 1 ;;
    esac
    offset=$(od -An -v -tu8 -j8 -N8 "$output" | tr -d ' \n')
    if [ "$offset" != 4096 ]; then
        echo "FAIL fault offset: $offset" >&2
        exit 1
    fi
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2 expected=$3
    mkdir -p "$expected"
    check_probe "$probe"
    "$probe" reference >"$expected/reference.bin"
    if [ "$(wc -c <"$expected/reference.bin")" -ne 23680 ]; then
        echo "FAIL reference output size" >&2
        exit 1
    fi
    normalise_reference "$expected/reference.bin" >"$expected/state.txt"
    if [ "$(wc -l <"$expected/state.txt")" -ne 740 ]; then
        echo "FAIL reference state count" >&2
        exit 1
    fi
    for name in $fault_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr"
        check_fault_record "$expected/$name.bin" "$expected/$name.status"
    done
    echo "PASS generated x86 vpsrlq reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2 probe=$3 expected=$4 source=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

    check_probe "$probe"
    check_reference_provenance "$expected"

    lsx_body=$(awk '
        /bool translate_vpsrlq_lsx\(/ { in_function = 1 }
        in_function { print }
        in_function && /^}/ { exit }
    ' "$source")
    printf '%s\n' "$lsx_body" | grep -Fq 'load_v128_from_ir1_mem_exact'
    if printf '%s\n' "$lsx_body" | grep -Eq '\bla_xv'; then
        echo "FAIL vpsrlq LSX function contains LASX" >&2
        exit 1
    fi
    lasx_body=$(awk '
        /static bool translate_vpsrlq_lasx\(/ { in_function = 1 }
        in_function { print }
        in_function && /^}/ { exit }
    ' "$source")
    printf '%s\n' "$lasx_body" | grep -Fq 'la_xvsrl_d'
    printf '%s\n' "$lasx_body" | grep -Fq 'la_bltu(count, max, label_shift)'
    printf '%s\n' "$lasx_body" | grep -Fq 'zero_ir2_opnd, 64'
    printf '%s\n' "$lasx_body" | grep -Fq 'load_v128_from_ir1_mem_exact'

    for mode in lasx lsx; do
        if [ "$mode" = lasx ]; then
            run_lasx_case "$latx" "$probe" reference \
                "$tmpdir/$mode.reference.bin" "$tmpdir/$mode.reference.stderr"
        else
            run_lsx_case "$latx" "$probe" reference \
                "$tmpdir/$mode.reference.bin" "$tmpdir/$mode.reference.stderr" \
                "$tmpdir/$mode.reference.gdb"
        fi
        printf '%s reference sha256=' "$mode"
        sha256sum "$tmpdir/$mode.reference.bin" | awk '{print $1}'

        for name in $fault_cases; do
            if [ "$mode" = lasx ]; then
                run_lasx_case "$latx" "$probe" "$name" \
                    "$tmpdir/$mode-$name.bin" "$tmpdir/$mode-$name.stderr"
            else
                run_lsx_case "$latx" "$probe" "$name" \
                    "$tmpdir/$mode-$name.bin" "$tmpdir/$mode-$name.stderr" \
                    "$tmpdir/$mode-$name.gdb"
            fi
            check_fault_case "$expected/$name.bin" \
                "$tmpdir/$mode-$name.bin" \
                "$tmpdir/$mode-$name.bin.status" "$mode"
        done
    done

    check_guest_reference "$expected/reference.bin" \
        "$tmpdir/lasx.reference.bin" lasx
    check_guest_reference "$expected/reference.bin" \
        "$tmpdir/lsx.reference.bin" lsx

    for mode in lasx lsx; do
        echo "INFO $mode trace fields are diagnostic and are not used for acceptance"
    done
    echo "PASS vpsrlq _lsx source, trusted reference, guest output and fault checks"
    ;;
*) usage ;;
esac
