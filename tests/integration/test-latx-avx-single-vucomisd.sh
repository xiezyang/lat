#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
fault_cases='load-cross-1 load-cross-7 load-cross-unmasked'
fpe_cases='fpe-invalid fpe-denormal fpe-invalid-priority'
no_signal_cases='no-signal-qnan-suppresses-denormal no-signal-daz no-signal-old-sticky no-signal-invalid-suppresses-denormal'

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
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vucomisd
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5 error=$6
    set +e
    if [ -n "$runner" ]; then
        env LATX_AVX_CPUID=0 "$runner" "$probe" "$name" \
            >"$output" 2>"$error"
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
    offset=$(od -An -v -tu8 -j8 -N8 "$output" | tr -d ' \n')
    if [ "$offset" != 4096 ]; then
        echo "FAIL fault offset: $offset" >&2
        exit 1
    fi
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
        fpe-invalid|fpe-invalid-priority) expected_code=7 ;;
        fpe-denormal) expected_code=5 ;;
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
}

compare_signal_rflags()
{
    signal_expected=$1 signal_actual=$2 signal_offset=$3 signal_name=$4
    expected_flags=$(od -An -v -tu8 -j "$signal_offset" -N8 \
        "$signal_expected" |
        tr -d ' \n')
    actual_flags=$(od -An -v -tu8 -j "$signal_offset" -N8 \
        "$signal_actual" |
        tr -d ' \n')
    expected_flags=$((expected_flags & ~65536))
    actual_flags=$((actual_flags & ~65536))
    if [ "$actual_flags" -ne "$expected_flags" ]; then
        echo "FAIL $signal_name RFLAGS excluding signal-frame RF: $actual_flags" >&2
        exit 1
    fi
}

check_sources()
{
    source=$1
    body=$(awk '
        /bool translate_vucomisd\(/ { in_function = 1; current = "" }
        in_function { current = current $0 "\n" }
        in_function && /^}/ { latest = current; in_function = 0 }
        END { printf "%s", latest }
    ' "$source")
    printf '%s\n' "$body" | grep -Fq 'if (1)'
    printf '%s\n' "$body" | grep -Fq 'vucomisd_load_scalar'
    printf '%s\n' "$body" | grep -Fq 'vucomisd_write_flags'
    printf '%s\n' "$body" | grep -Fq 'xcomisx'
    lsx_branch=$(printf '%s\n' "$body" | awk '
        /if \(1\)/ { in_branch = 1 }
        in_branch && /} else {/ { exit }
        in_branch { print }
    ')
    if printf '%s\n' "$lsx_branch" | grep -Eq '\bla_xv|la_fcmp'; then
        echo "FAIL vucomisd LSX branch contains LASX or host FP compare" >&2
        exit 1
    fi
}

check_host_code()
{
    latx=$1
    dump=$(objdump -d "$latx" |
        sed -n '/<translate_vucomisd>:/,/^[[:space:]]*$/p')
    if printf '%s\n' "$dump" |
       grep -Eq '<xcomisx>|<la_xv[^>]*>|<la_fcmp[^>]*>'; then
        echo "FAIL compiled vucomisd reaches old LASX/host FP generator" >&2
        exit 1
    fi
    printf '%s\n' "$dump" | grep -Fq '<vucomisd_load_scalar>'
    printf '%s\n' "$dump" | grep -Fq '<la_x86mtflag>'
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2 expected=$3
    mkdir -p "$expected"
    check_probe "$probe"
    "$probe" reference >"$expected/reference.bin"
    if [ "$(wc -c <"$expected/reference.bin")" -ne 2688 ]; then
        echo "FAIL reference output size: $(wc -c <"$expected/reference.bin")" >&2
        exit 1
    fi
    unique_flags=$(od -An -v -tx8 -w32 "$expected/reference.bin" |
        awk '{ print $1 }' | sort -u | wc -l)
    unique_mxcsr=$(od -An -v -tx4 -w32 "$expected/reference.bin" |
        awk '{ print $3 }' | sort -u | wc -l)
    if [ "$unique_flags" -lt 4 ] || [ "$unique_mxcsr" -lt 4 ]; then
        echo "FAIL reference diversity: flags=$unique_flags mxcsr=$unique_mxcsr" >&2
        exit 1
    fi
    for name in $fault_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr"
        check_fault_record "$expected/$name.bin" "$expected/$name.status"
    done
    for name in $fpe_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr"
        check_fpe_record "$expected/$name.bin" \
            "$expected/$name.status" "$name"
    done
    for name in $no_signal_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr"
        if [ "$(sed -n '1p' "$expected/$name.status")" != 0 ] ||
           [ "$(wc -c <"$expected/$name.bin")" -ne 32 ]; then
            echo "FAIL $name did not complete normally" >&2
            exit 1
        fi
    done
    echo "PASS generated x86 vucomisd reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2 probe=$3 expected=$4 source=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

    check_probe "$probe"
    env LATX_AVX_CPUID=0 "$latx" "$probe" actual \
        >"$tmpdir/actual.bin"
    cmp "$expected/reference.bin" "$tmpdir/actual.bin"

    for name in $fault_cases; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" \
            "$tmpdir/$name.stderr"
        check_fault_record "$tmpdir/$name.bin" "$tmpdir/$name.status"
        cmp -n 40 "$expected/$name.bin" "$tmpdir/$name.bin"
        compare_signal_rflags "$expected/$name.bin" \
            "$tmpdir/$name.bin" 40 "$name"
        cmp -i 48 "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
    done
    for name in $fpe_cases; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" \
            "$tmpdir/$name.stderr"
        check_fpe_record "$tmpdir/$name.bin" \
            "$tmpdir/$name.status" "$name"
        cmp -n 32 "$expected/$name.bin" "$tmpdir/$name.bin"
        compare_signal_rflags "$expected/$name.bin" \
            "$tmpdir/$name.bin" 32 "$name"
        cmp -i 40 "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
    done
    for name in $no_signal_cases; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" \
            "$tmpdir/$name.stderr"
        cmp "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
    done

    check_sources "$source"
    check_host_code "$latx"
    echo "PASS vucomisd RFLAGS, MXCSR, register/m64, SIGFPE and fault differential"
    ;;
*) usage ;;
esac
