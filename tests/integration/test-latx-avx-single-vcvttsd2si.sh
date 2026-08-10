#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
fault_cases='load-cross-1 load-cross-7 load-unreadable'
fpe_cases='fpe-invalid fpe-precision fpe-subnormal-precision'
no_signal_cases='no-signal-daz no-signal-old-ie no-signal-old-pe'

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
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vcvttsd2si
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5 error=$6 cvt_opt=$7
    set +e
    if [ -n "$runner" ]; then
        env LATX_AVX_CPUID="${LATX_AVX_CPUID_VALUE:-0}" LATX_CVT_OPT="$cvt_opt" \
            "$runner" "$probe" "$name" >"$output" 2>"$error"
    else
        "$probe" "$name" >"$output" 2>"$error"
    fi
    status=$?
    set -e
    printf '%s\n' "$status" >"$status_file"
}

check_fault_record()
{
    output=$1 status_file=$2
    if [ "$(sed -n '1p' "$status_file")" != 139 ]; then
        echo "FAIL fault status: $(sed -n '1p' "$status_file")" >&2
        exit 1
    fi
    if [ "$(wc -c <"$output")" -ne 64 ]; then
        echo "FAIL fault record size: $(wc -c <"$output")" >&2
        exit 1
    fi
    header=$(od -An -v -tx4 -N8 "$output" | tr -d ' \n')
    case "$header" in
        0000000b00000002) ;;
        *) echo "FAIL fault signal/code: $header" >&2; exit 1 ;;
    esac
}

check_fpe_record()
{
    output=$1 status_file=$2 name=$3
    if [ "$(sed -n '1p' "$status_file")" != 136 ]; then
        echo "FAIL $name status: $(sed -n '1p' "$status_file")" >&2
        exit 1
    fi
    if [ "$(wc -c <"$output")" -ne 64 ]; then
        echo "FAIL $name record size: $(wc -c <"$output")" >&2
        exit 1
    fi
    signal_number=$(od -An -v -td4 -N4 "$output" | tr -d ' \n')
    signal_code=$(od -An -v -td4 -j4 -N4 "$output" | tr -d ' \n')
    case "$name" in
        fpe-invalid) expected_code=7 ;;
        fpe-precision|fpe-subnormal-precision) expected_code=6 ;;
        *) echo "FAIL unknown FPE case: $name" >&2; exit 1 ;;
    esac
    if [ "$signal_number" != 8 ] || [ "$signal_code" != "$expected_code" ]; then
        echo "FAIL $name signal/code: $signal_number/$signal_code" >&2
        exit 1
    fi
    signal_offset=$(od -An -v -td8 -j8 -N8 "$output" | tr -d ' \n')
    rip_offset=$(od -An -v -td8 -j16 -N8 "$output" | tr -d ' \n')
    if [ "$signal_offset" != 0 ] || [ "$rip_offset" != 0 ]; then
        echo "FAIL $name signal/RIP offset: $signal_offset/$rip_offset" >&2
        exit 1
    fi
    r15=$(od -An -v -tx8 -j40 -N8 "$output" | tr -d ' \n')
    if [ "$r15" != 1122334455667788 ]; then
        echo "FAIL $name committed target r15: $r15" >&2
        exit 1
    fi
}

compare_signal_rflags()
{
    signal_expected=$1 signal_actual=$2 signal_offset=$3 signal_name=$4
    expected_flags=$(od -An -v -tu8 -j "$signal_offset" -N8 \
        "$signal_expected" | tr -d ' \n')
    actual_flags=$(od -An -v -tu8 -j "$signal_offset" -N8 \
        "$signal_actual" | tr -d ' \n')
    expected_flags=$((expected_flags & ~65536))
    actual_flags=$((actual_flags & ~65536))
    if [ "$actual_flags" -ne "$expected_flags" ]; then
        echo "FAIL $signal_name RFLAGS excluding signal-frame RF: $actual_flags" >&2
        exit 1
    fi
}

compare_fault_record()
{
    expected=$1 actual=$2 name=$3
    cmp -n 16 "$expected" "$actual"
    compare_signal_rflags "$expected" "$actual" 16 "$name"
    cmp -i 24 "$expected" "$actual"
}

compare_fpe_record()
{
    expected=$1 actual=$2 name=$3
    cmp -n 32 "$expected" "$actual"
    compare_signal_rflags "$expected" "$actual" 32 "$name"
    cmp -i 40 "$expected" "$actual"
}

check_sources()
{
    source=$1
    lsx_body=$(sed -n '/bool translate_vcvttsd2si_lsx(/,/^}/p' "$source")
    printf '%s\n' "$lsx_body" | grep -Fq 'vcvttsd2si_load_scalar'
    printf '%s\n' "$lsx_body" | grep -Fq 'vcvttsd2si_convert_bits'
    printf '%s\n' "$lsx_body" | grep -Fq 'helper_raise_simd_exception'
    if printf '%s\n' "$lsx_body" |
       grep -Eq '\bla_xv|la_fcmp|la_ftintrz|la_vftintrz'; then
        echo "FAIL vcvttsd2si LSX branch uses LASX or host FP conversion" >&2
        exit 1
    fi
}

check_host_code()
{
    latx=$1
    dump=$(objdump -d "$latx" |
        sed -n '/<translate_vcvttsd2si_lsx[^>]*>:/,/^[[:space:]]*$/p')
    if [ -z "$dump" ]; then
        echo "FAIL missing compiled translate_vcvttsd2si_lsx" >&2
        exit 1
    fi
    if printf '%s\n' "$dump" |
       grep -Eq '<la_xv[^>]*>|<la_fcmp[^>]*>|<la_ftintrz[^>]*>|<la_vftintrz[^>]*>'; then
        echo "FAIL compiled vcvttsd2si uses LASX or host FP conversion" >&2
        exit 1
    fi
    printf '%s\n' "$dump" | grep -Fq '<la_vpickve2gr_du>'
    printf '%s\n' "$dump" | grep -Fq '<load_u64_from_ir1_mem_exact>'
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2 expected=$3
    mkdir -p "$expected"
    check_probe "$probe"
    "$probe" reference >"$expected/reference.bin"
    size=$(wc -c <"$expected/reference.bin")
    if [ "$size" -lt 2048 ] || [ $((size % 32)) -ne 0 ]; then
        echo "FAIL reference output size: $size" >&2
        exit 1
    fi
    unique_results=$(od -An -v -tx8 -w32 "$expected/reference.bin" |
        awk '{ print $1 }' | sort -u | wc -l)
    unique_mxcsr=$(od -An -v -tx4 -w32 "$expected/reference.bin" |
        awk '{ print $3 }' | sort -u | wc -l)
    if [ "$unique_results" -lt 8 ] || [ "$unique_mxcsr" -lt 4 ]; then
        echo "FAIL reference diversity: result=$unique_results mxcsr=$unique_mxcsr" >&2
        exit 1
    fi
    for name in $fault_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr" 1
        check_fault_record "$expected/$name.bin" "$expected/$name.status"
    done
    for name in $fpe_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr" 1
        check_fpe_record "$expected/$name.bin" \
            "$expected/$name.status" "$name"
    done
    for name in $no_signal_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr" 1
        if [ "$(sed -n '1p' "$expected/$name.status")" != 0 ] ||
           [ "$(wc -c <"$expected/$name.bin")" -ne 32 ]; then
            echo "FAIL $name did not complete normally" >&2
            exit 1
        fi
    done
    echo "PASS generated x86 vcvttsd2si reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2 probe=$3 expected_dir=$4 source=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

    check_probe "$probe"
    for cvt_opt in 0 1; do
        env LATX_AVX_CPUID="${LATX_AVX_CPUID_VALUE:-0}" LATX_CVT_OPT="$cvt_opt" \
            "$latx" "$probe" actual >"$tmpdir/actual-$cvt_opt.bin"
        cmp "$expected_dir/reference.bin" "$tmpdir/actual-$cvt_opt.bin"

        for name in $fault_cases; do
            run_case "$latx" "$probe" "$name" \
                "$tmpdir/$name-$cvt_opt.bin" \
                "$tmpdir/$name-$cvt_opt.status" \
                "$tmpdir/$name-$cvt_opt.stderr" "$cvt_opt"
            check_fault_record "$tmpdir/$name-$cvt_opt.bin" \
                "$tmpdir/$name-$cvt_opt.status"
            compare_fault_record "$expected_dir/$name.bin" \
                "$tmpdir/$name-$cvt_opt.bin" "$name"
            cmp "$expected_dir/$name.status" \
                "$tmpdir/$name-$cvt_opt.status"
        done
        for name in $fpe_cases; do
            run_case "$latx" "$probe" "$name" \
                "$tmpdir/$name-$cvt_opt.bin" \
                "$tmpdir/$name-$cvt_opt.status" \
                "$tmpdir/$name-$cvt_opt.stderr" "$cvt_opt"
            check_fpe_record "$tmpdir/$name-$cvt_opt.bin" \
                "$tmpdir/$name-$cvt_opt.status" "$name"
            compare_fpe_record "$expected_dir/$name.bin" \
                "$tmpdir/$name-$cvt_opt.bin" "$name"
            cmp "$expected_dir/$name.status" \
                "$tmpdir/$name-$cvt_opt.status"
        done
        for name in $no_signal_cases; do
            run_case "$latx" "$probe" "$name" \
                "$tmpdir/$name-$cvt_opt.bin" \
                "$tmpdir/$name-$cvt_opt.status" \
                "$tmpdir/$name-$cvt_opt.stderr" "$cvt_opt"
            cmp "$expected_dir/$name.bin" \
                "$tmpdir/$name-$cvt_opt.bin"
            cmp "$expected_dir/$name.status" \
                "$tmpdir/$name-$cvt_opt.status"
        done
    done

    check_sources "$source"
    check_host_code "$latx"
    echo "PASS vcvttsd2si W0/W1, GPR/XMM, m64, MXCSR, SIGFPE and fault differential"
    ;;
*) usage ;;
esac
