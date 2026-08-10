#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
fault_cases='load-cross-1 load-cross-7 load-cross-unmasked'
fpe_cases='fpe-invalid fpe-denormal fpe-overflow fpe-underflow fpe-precision fpe-overflow-precision fpe-underflow-precision fpe-denormal-priority fpe-invalid-priority fpe-underflow-ftz'
no_signal_cases='no-signal-daz no-signal-old-sticky'

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
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vmulsd
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5 error=$6
    argument=$name
    set +e
    if [ -n "$runner" ]; then
        argument=trace-$name
        env LATX_AVX_CPUID=1 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
            LATX_AVX_TRACE_YMM_INIT=1 \
            "$runner" "$probe" "$argument" >"$output" 2>"$error"
    else
        "$probe" "$argument" >"$output" 2>"$error"
    fi
    status=$?
    set -e
    printf '%s\n' "$status" >"$status_file"
}

normalise_reference()
{
    input=$1
    size=$(wc -c <"$input")
    offset=0

    while [ "$offset" -lt "$size" ]; do
        od -An -v -tx8 -w32 -j "$offset" -N32 "$input" |
            awk '{ print $1, $2, $3, $4 }'
        offset=$((offset + 64))
    done
}

normalise_arch_state()
{
    input=$1
    size=$(wc -c <"$input")
    offset=0
    index=1

    while [ "$offset" -lt "$size" ]; do
        low=$(od -An -v -tx8 -w16 -j "$offset" -N16 "$input" |
            awk '{ print $1, $2 }')
        mxcsr=$(od -An -v -tx4 -j $((offset + 32)) -N4 "$input" |
            tr -d ' \n')
        printf '%02d %s %s\n' "$index" "$low" "$mxcsr"
        offset=$((offset + 64))
        index=$((index + 1))
    done
}

marker_pc()
{
    nm -n "$1" | awk '$3 == "latx_avx_single_vmulsd_observe_marker" {
        sub(/^0+/, "", $1)
        print "0x" $1
    }'
}

normalise_trace()
{
    trace=$1 marker=$2
    awk -v marker="$marker" '
        /event=hit/ {
            capture = saw_marker
            saw_marker = index($0, "pc=" marker " ") != 0
            next
        }
        capture && /event=ymm_state/ {
            for (i = 1; i <= NF; ++i) {
                split($i, field, "=")
                if (field[1] == "low0") low0 = field[2]
                if (field[1] == "low1") low1 = field[2]
                if (field[1] == "shadow_high0") high0 = field[2]
                if (field[1] == "shadow_high1") high1 = field[2]
            }
            print low0, low1, high0, high1
            capture = 0
        }
    ' "$trace"
}

normalise_faulting_trace()
{
    trace=$1
    awk '
        /event=ymm_state/ {
            for (i = 1; i <= NF; ++i) {
                split($i, field, "=")
                if (field[1] == "low0") low0 = field[2]
                if (field[1] == "low1") low1 = field[2]
                if (field[1] == "shadow_high0") high0 = field[2]
                if (field[1] == "shadow_high1") high1 = field[2]
            }
            print low0, low1, high0, high1
        }
    ' "$trace"
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

check_reference_fault_state()
{
    output=$1 expected_mxcsr=$2
    xmm=$(od -An -v -tx8 -j16 -N16 "$output" |
        awk '{ print $1, $2 }')
    high=$(od -An -v -tx8 -j32 -N16 "$output" |
        awk '{ print $1, $2 }')
    mxcsr=$(od -An -v -tx4 -j48 -N4 "$output" | tr -d ' \n')
    if [ "$xmm" != "6f13b88ac09d5e27 e4a2097c31d6f850" ]; then
        echo "FAIL fault changed XMM15: $xmm" >&2
        exit 1
    fi
    if [ "$high" != "1122334455667788 99aabbccddeeff00" ]; then
        echo "FAIL fault changed YMM15 high: $high" >&2
        exit 1
    fi
    if [ "$mxcsr" != "$expected_mxcsr" ]; then
        echo "FAIL fault changed MXCSR: $mxcsr" >&2
        exit 1
    fi
}

check_fpe_record()
{
    output=$1 status_file=$2 name=$3 check_high=${4:-1}
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
        fpe-denormal|fpe-denormal-priority|fpe-underflow|fpe-underflow-ftz)
            expected_code=5 ;;
        fpe-overflow) expected_code=4 ;;
        fpe-precision|fpe-overflow-precision|fpe-underflow-precision)
            expected_code=6 ;;
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
    xmm=$(od -An -v -tx8 -j32 -N16 "$output" |
        awk '{ print $1, $2 }')
    high=$(od -An -v -tx8 -j48 -N16 "$output" |
        awk '{ print $1, $2 }')
    if [ "$xmm" != "6f13b88ac09d5e27 e4a2097c31d6f850" ]; then
        echo "FAIL $name changed XMM15: $xmm" >&2
        exit 1
    fi
    if [ "$check_high" = 1 ] &&
       [ "$high" != "1122334455667788 99aabbccddeeff00" ]; then
        echo "FAIL $name changed YMM15 high: $high" >&2
        exit 1
    fi
}

check_sources()
{
    source=$1
    lsx_body=$(awk '
        /bool translate_vmulsd_lsx\(/ { capture = 1 }
        capture {
            print
            opens += gsub(/\{/, "{")
            closes += gsub(/\}/, "}")
            if (opens > 0 && opens == closes) exit
        }
    ' "$source")
    lasx_body=$(awk '
        /bool translate_vmulsd\(/ { capture = 1 }
        capture {
            print
            opens += gsub(/\{/, "{")
            closes += gsub(/\}/, "}")
            if (opens > 0 && opens == closes) exit
        }
    ' "$source")
    printf '%s\n' "$lsx_body" | grep -Fq 'load_u64_from_ir1_mem_exact'
    printf '%s\n' "$lsx_body" | grep -Fq 'la_fmul_d'
    printf '%s\n' "$lsx_body" | grep -Fq 'la_vpickve2gr_du'
    printf '%s\n' "$lsx_body" | grep -Fq 'la_vinsgr2vr_d'
    printf '%s\n' "$lsx_body" | grep -Fq 'clear_ymm_high128_shadow'
    if printf '%s\n' "$lsx_body" | grep -Eq '\bla_xv'; then
        echo "FAIL vmulsd LSX function contains LASX generator" >&2
        exit 1
    fi
    printf '%s\n' "$lasx_body" | grep -Fq 'la_fmul_d'
    printf '%s\n' "$lasx_body" | grep -Fq 'la_xvori_b'
    printf '%s\n' "$lasx_body" | grep -Fq 'clear_ymm_high128_shadow'
}

check_host_code()
{
    latx=$1
    lsx_dump=$(objdump -d "$latx" | awk '
        /<translate_vmulsd_lsx>:/ { in_function = 1 }
        in_function && /<translate_vmulsd_lsx>:/ { print; next }
        in_function && /^$/ { exit }
        in_function && /^[[:space:]]*[[:xdigit:]]+ <[^>]+>:/ { exit }
        in_function { print }
    ')
    lasx_dump=$(objdump -d "$latx" | awk '
        /<translate_vmulsd>:/ { in_function = 1 }
        in_function && /<translate_vmulsd>:/ { print; next }
        in_function && /^$/ { exit }
        in_function && /^[[:space:]]*[[:xdigit:]]+ <[^>]+>:/ { exit }
        in_function { print }
    ')
    if printf '%s\n' "$lsx_dump" | grep -Eq '<la_xv[^>]*>'; then
        echo "FAIL compiled vmulsd_lsx calls LASX generator" >&2
        exit 1
    fi
    printf '%s\n' "$lsx_dump" | grep -Fq '<load_u64_from_ir1_mem_exact>'
    printf '%s\n' "$lsx_dump" | grep -Fq '<la_fmul_d>'
    printf '%s\n' "$lsx_dump" | grep -Fq '<la_vpickve2gr_du>'
    printf '%s\n' "$lsx_dump" | grep -Fq '<la_vinsgr2vr_d>'
    printf '%s\n' "$lasx_dump" | grep -Fq '<la_xvori_b>'
    printf '%s\n' "$lasx_dump" | grep -Fq '<la_fmul_d>'
}

verify_trace()
{
    trace_latx=$1 trace_probe=$2 trace_marker=$3 trace_reg=$4
    trace_mode=$5 trace_expected=$6 trace_count=$7 trace_tmpdir=$8
    env LATX_AVX_CPUID=1 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM="$trace_reg" \
        LATX_AVX_TRACE_YMM_INIT=1 \
        "$trace_latx" "$trace_probe" "$trace_mode" \
        >"$trace_tmpdir/$trace_mode.stdout" \
        2>"$trace_tmpdir/$trace_mode.log"
    normalise_trace "$trace_tmpdir/$trace_mode.log" "$trace_marker" \
        >"$trace_tmpdir/$trace_mode.state"
    if [ "$(wc -l <"$trace_tmpdir/$trace_mode.state")" -ne "$trace_count" ]; then
        echo "FAIL $trace_mode trace state count: $(wc -l <"$trace_tmpdir/$trace_mode.state")" >&2
        exit 1
    fi
    cmp "$trace_expected" "$trace_tmpdir/$trace_mode.state"
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2 expected=$3
    mkdir -p "$expected"
    check_probe "$probe"
    "$probe" reference >"$expected/reference.bin"
    if [ "$(wc -c <"$expected/reference.bin")" -ne 6656 ]; then
        echo "FAIL reference output size" >&2
        exit 1
    fi
    normalise_reference "$expected/reference.bin" >"$expected/state.txt"
    normalise_arch_state "$expected/reference.bin" >"$expected/arch-state.txt"
    if [ "$(wc -l <"$expected/state.txt")" -ne 104 ]; then
        echo "FAIL reference state count" >&2
        exit 1
    fi
    unique_states=$(sort -u "$expected/state.txt" | wc -l)
    if [ "$unique_states" -lt 18 ]; then
        echo "FAIL reference state diversity: $unique_states" >&2
        exit 1
    fi
    unique_mxcsr=$(awk '{ print $4 }' "$expected/arch-state.txt" |
        sort -u | wc -l)
    if [ "$unique_mxcsr" -lt 8 ]; then
        echo "FAIL MXCSR state diversity: $unique_mxcsr" >&2
        exit 1
    fi
    sed -n '1,44p' "$expected/state.txt" \
        >"$expected/state-reg0-register.txt"
    sed -n '45,52p' "$expected/state.txt" \
        >"$expected/state-reg0-memory.txt"
    sed -n '53,96p' "$expected/state.txt" \
        >"$expected/state-reg15-register.txt"
    sed -n '97,104p' "$expected/state.txt" \
        >"$expected/state-reg15-memory.txt"
    for name in $fault_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr"
        check_fault_record "$expected/$name.bin" "$expected/$name.status"
        expected_mxcsr=00001f80
        if [ "$name" = load-cross-unmasked ]; then
            expected_mxcsr=00001f00
        fi
        check_reference_fault_state "$expected/$name.bin" "$expected_mxcsr"
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
           [ "$(wc -c <"$expected/$name.bin")" -ne 64 ]; then
            echo "FAIL $name did not complete normally" >&2
            exit 1
        fi
    done
    echo "PASS generated x86 vmulsd reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2 probe=$3 expected=$4 source=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

    check_probe "$probe"
    marker=$(marker_pc "$probe")
    if [ -z "$marker" ]; then
        echo "FAIL missing vmulsd observer marker" >&2
        exit 1
    fi

    env LATX_AVX_CPUID=0 "$latx" "$probe" actual \
        >"$tmpdir/actual.bin"
    if [ "$(wc -c <"$tmpdir/actual.bin")" -ne 6656 ]; then
        echo "FAIL LATX result output size" >&2
        exit 1
    fi
    normalise_arch_state "$tmpdir/actual.bin" >"$tmpdir/arch-state.txt"
    cmp "$expected/arch-state.txt" "$tmpdir/arch-state.txt"

    verify_trace "$latx" "$probe" "$marker" 0 trace0-register \
        "$expected/state-reg0-register.txt" 44 "$tmpdir"
    verify_trace "$latx" "$probe" "$marker" 0 trace0-memory \
        "$expected/state-reg0-memory.txt" 8 "$tmpdir"
    verify_trace "$latx" "$probe" "$marker" 15 trace15-register \
        "$expected/state-reg15-register.txt" 44 "$tmpdir"
    verify_trace "$latx" "$probe" "$marker" 15 trace15-memory \
        "$expected/state-reg15-memory.txt" 8 "$tmpdir"

    for name in $fault_cases; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" \
            "$tmpdir/$name.log"
        check_fault_record "$tmpdir/$name.bin" "$tmpdir/$name.status"
        cmp -n 32 "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
        expected_mxcsr=$(od -An -v -tx4 -j48 -N4 \
            "$expected/$name.bin" | tr -d ' \n')
        actual_mxcsr=$(od -An -v -tx4 -j48 -N4 \
            "$tmpdir/$name.bin" | tr -d ' \n')
        if [ "$actual_mxcsr" != "$expected_mxcsr" ]; then
            echo "FAIL $name changed MXCSR: $actual_mxcsr" >&2
            exit 1
        fi
        normalise_trace "$tmpdir/$name.log" "$marker" \
            >"$tmpdir/$name.state"
        expected_high=$(od -An -v -tx8 -j32 -N16 \
            "$expected/$name.bin" | awk '{ print $1, $2 }')
        actual_high=$(awk '{ print $3, $4 }' "$tmpdir/$name.state")
        if [ "$actual_high" != "$expected_high" ]; then
            echo "FAIL $name changed YMM15 high: $actual_high" >&2
            exit 1
        fi
    done

    for name in $fpe_cases; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" \
            "$tmpdir/$name.log"
        check_fpe_record \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" "$name" 0
        cmp -n 48 "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
        normalise_faulting_trace "$tmpdir/$name.log" \
            >"$tmpdir/$name.state"
        if [ "$(wc -l <"$tmpdir/$name.state")" -ne 1 ]; then
            echo "FAIL $name trace state count" >&2
            exit 1
        fi
        expected_state=$(od -An -v -tx8 -w32 -j32 -N32 \
            "$expected/$name.bin" | awk '{ print $1, $2, $3, $4 }')
        actual_state=$(sed -n '1p' "$tmpdir/$name.state")
        if [ "$actual_state" != "$expected_state" ]; then
            echo "FAIL $name trace target state: $actual_state" >&2
            exit 1
        fi
    done

    for name in $no_signal_cases; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" \
            "$tmpdir/$name.log"
        cmp -n 16 "$expected/$name.bin" "$tmpdir/$name.bin"
        expected_mxcsr=$(od -An -v -tx4 -j32 -N4 \
            "$expected/$name.bin" | tr -d ' \n')
        actual_mxcsr=$(od -An -v -tx4 -j32 -N4 \
            "$tmpdir/$name.bin" | tr -d ' \n')
        if [ "$actual_mxcsr" != "$expected_mxcsr" ]; then
            echo "FAIL $name MXCSR: $actual_mxcsr" >&2
            exit 1
        fi
        cmp "$expected/$name.status" "$tmpdir/$name.status"
        normalise_trace "$tmpdir/$name.log" "$marker" \
            >"$tmpdir/$name.state"
        if [ "$(wc -l <"$tmpdir/$name.state")" -ne 1 ]; then
            echo "FAIL $name trace state count" >&2
            exit 1
        fi
        expected_state=$(od -An -v -tx8 -w32 -N32 \
            "$expected/$name.bin" | awk '{ print $1, $2, $3, $4 }')
        actual_state=$(sed -n '1p' "$tmpdir/$name.state")
        if [ "$actual_state" != "$expected_state" ]; then
            echo "FAIL $name trace target state: $actual_state" >&2
            exit 1
        fi
    done

    check_sources "$source"
    check_host_code "$latx"
    echo "PASS vmulsd register aliases, scalar FP/MXCSR, unmasked SIGFPE, m64 addressing and fault differential"
    ;;
*) usage ;;
esac
