#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
fault_cases='load-cross-1 load-cross-7 store-cross-1 store-cross-7'

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
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vmovsd
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5 error=$6
    argument=$name
    set +e
    if [ -n "$runner" ]; then
        argument=trace-$name
        env LATX_AVX_CPUID=1 LATX_AOT=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
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

marker_pc()
{
    nm -n "$1" | awk '$3 == "latx_avx_single_vmovsd_observe_marker" {
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

check_sources()
{
    source=$1
    extract_function()
    {
        symbol=$1
        awk -v symbol="$symbol" '
            index($0, symbol) > 0 {
                capture = 1
                depth = 0
                opened = 0
                current = ""
            }
            capture {
                current = current $0 "\n"
                opens = gsub(/\{/, "{")
                closes = gsub(/\}/, "}")
                depth += opens - closes
                if (opens > 0) {
                    opened = 1
                }
                if (opened && depth == 0) {
                    latest = current
                    capture = 0
                }
            }
            END { printf "%s", latest }
        ' "$source"
    }

    lsx_body=$(extract_function 'bool translate_vmovsd_lsx(')
    lasx_body=$(extract_function 'bool translate_vmovsd(')
    [ -n "$lsx_body" ] || { echo "FAIL missing translate_vmovsd_lsx" >&2; exit 1; }
    [ -n "$lasx_body" ] || { echo "FAIL missing translate_vmovsd" >&2; exit 1; }
    if printf '%s\n' "$lsx_body" | grep -Eq '\bla_xv|option_enable_lasx|if[[:space:]]*\([[:space:]]*1[[:space:]]*\)'; then
        echo "FAIL vmovsd LSX function contains LASX or backend selection" >&2
        exit 1
    fi
    if printf '%s\n' "$lasx_body" | grep -Eq 'option_enable_lasx|if[[:space:]]*\([[:space:]]*1[[:space:]]*\)|translate_vmovsd_lsx'; then
        echo "FAIL vmovsd LASX function contains backend selection" >&2
        exit 1
    fi
    for required in load_u64_from_ir1_mem_exact store_u64_to_ir1_mem_exact \
                    la_vinsgr2vr_d la_vpickve2gr_du clear_ymm_high128_shadow; do
        printf '%s\n' "$lsx_body" | grep -Fq "$required" || {
            echo "FAIL vmovsd LSX function misses $required" >&2
            exit 1
        }
    done
    printf '%s\n' "$lasx_body" | grep -Eq '\bla_xv' || {
        echo "FAIL vmovsd LASX function has no LASX generator" >&2
        exit 1
    }
    grep -Fq 'TRANS_FUNC_DEF(vmovsd_lsx)' target/i386/latx/include/translate.h
    grep -Fq 'translate_register_lsx(dt_X86_INS_VMOVSD, translate_vmovsd_lsx)' \
        target/i386/latx/translator/translate.c
}

check_host_code()
{
    latx=$1
    dump=$(objdump -d --disassemble=translate_vmovsd_lsx "$latx" | awk '
        /<translate_vmovsd_lsx>:/ { in_function = 1 }
        in_function && /<translate_vmovsd_lsx>:/ { print; next }
        in_function && /^$/ { exit }
        in_function && /^[[:space:]]*[[:xdigit:]]+ <[^>]+>:/ { exit }
        in_function { print }
    ')
    if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>'; then
        echo "FAIL compiled translate_vmovsd_lsx calls LASX generator" >&2
        exit 1
    fi
    printf '%s\n' "$dump" | grep -Fq '<load_u64_from_ir1_mem_exact>'
    printf '%s\n' "$dump" | grep -Fq '<store_u64_to_ir1_mem_exact>'
    printf '%s\n' "$dump" | grep -Fq '<la_vinsgr2vr_d>'
    printf '%s\n' "$dump" | grep -Fq '<la_vpickve2gr_du>'
}

verify_trace()
{
    trace_latx=$1 trace_probe=$2 trace_marker=$3 trace_reg=$4
    trace_mode=$5 trace_expected=$6 trace_count=$7 trace_tmpdir=$8
    env LATX_AVX_CPUID=1 LATX_AOT=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM="$trace_reg" \
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
    if [ "$(wc -c <"$expected/reference.bin")" -ne 2688 ]; then
        echo "FAIL reference output size" >&2
        exit 1
    fi
    normalise_reference "$expected/reference.bin" >"$expected/state.txt"
    if [ "$(wc -l <"$expected/state.txt")" -ne 42 ]; then
        echo "FAIL reference state count" >&2
        exit 1
    fi
    unique_states=$(sort -u "$expected/state.txt" | wc -l)
    if [ "$unique_states" -lt 10 ]; then
        echo "FAIL reference state diversity: $unique_states" >&2
        exit 1
    fi
    sed -n '1,8p' "$expected/state.txt" >"$expected/state-reg0-store.txt"
    sed -n '9,16p' "$expected/state.txt" >"$expected/state-reg0-load.txt"
    sed -n '17,21p' "$expected/state.txt" >"$expected/state-reg0-register.txt"
    sed -n '22,29p' "$expected/state.txt" >"$expected/state-reg15-store.txt"
    sed -n '30,37p' "$expected/state.txt" >"$expected/state-reg15-load.txt"
    sed -n '38,42p' "$expected/state.txt" >"$expected/state-reg15-register.txt"
    "$probe" memory0 >"$expected/memory0.bin"
    "$probe" memory15 >"$expected/memory15.bin"
    if [ "$(wc -c <"$expected/memory0.bin")" -ne 256 ] ||
       [ "$(wc -c <"$expected/memory15.bin")" -ne 256 ]; then
        echo "FAIL store snapshot size" >&2
        exit 1
    fi
    for name in $fault_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr"
        check_fault_record "$expected/$name.bin" "$expected/$name.status"
    done
    echo "PASS generated x86 vmovsd reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2 probe=$3 expected=$4 source=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

    check_probe "$probe"
    marker=$(marker_pc "$probe")
    if [ -z "$marker" ]; then
        echo "FAIL missing vmovsd observer marker" >&2
        exit 1
    fi
    verify_trace "$latx" "$probe" "$marker" 0 trace0-store \
        "$expected/state-reg0-store.txt" 8 "$tmpdir"
    verify_trace "$latx" "$probe" "$marker" 0 trace0-load \
        "$expected/state-reg0-load.txt" 8 "$tmpdir"
    verify_trace "$latx" "$probe" "$marker" 0 trace0-register \
        "$expected/state-reg0-register.txt" 5 "$tmpdir"
    verify_trace "$latx" "$probe" "$marker" 15 trace15-store \
        "$expected/state-reg15-store.txt" 8 "$tmpdir"
    verify_trace "$latx" "$probe" "$marker" 15 trace15-load \
        "$expected/state-reg15-load.txt" 8 "$tmpdir"
    verify_trace "$latx" "$probe" "$marker" 15 trace15-register \
        "$expected/state-reg15-register.txt" 5 "$tmpdir"

    env LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" memory0 \
        >"$tmpdir/memory0.bin"
    env LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" memory15 \
        >"$tmpdir/memory15.bin"
    cmp "$expected/memory0.bin" "$tmpdir/memory0.bin"
    cmp "$expected/memory15.bin" "$tmpdir/memory15.bin"

    for name in $fault_cases; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" \
            "$tmpdir/$name.log"
        check_fault_record "$tmpdir/$name.bin" "$tmpdir/$name.status"
        cmp -n 32 "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
        expected_memory=$(od -An -v -tx8 -j48 -N16 \
            "$expected/$name.bin" | awk '{ print $1, $2 }')
        actual_memory=$(od -An -v -tx8 -j48 -N16 \
            "$tmpdir/$name.bin" | awk '{ print $1, $2 }')
        if [ "$actual_memory" != "$expected_memory" ]; then
            echo "FAIL $name changed accessible memory: $actual_memory" >&2
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

    check_sources "$source"
    check_host_code "$latx"
    echo "PASS vmovsd three-register aliases, m64 load/store, addressing and fault differential"
    ;;
*) usage ;;
esac
