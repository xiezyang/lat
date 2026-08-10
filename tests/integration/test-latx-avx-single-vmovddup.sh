#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
fault_cases='m64-cross-1 m64-cross-7 ymm-cross-1 ymm-cross-15 ymm-cross-16 ymm-cross-31'

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
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vmovddup
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5 error=$6
    set +e
    if [ -n "$runner" ]; then
        env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
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

marker_pc()
{
    nm -n "$1" | awk '$3 == "latx_avx_single_vmovddup_observe_marker" {
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

check_sources()
{
    source=$1
    body=$(awk '
        /bool translate_vmovddup\(/ { in_function = 1 }
        in_function { print }
        in_function && /^}/ { exit }
    ' "$source")
    printf '%s\n' "$body" | grep -Fq 'if (1)'
    printf '%s\n' "$body" | grep -Fq 'load_u64_from_ir1_mem_exact'
    printf '%s\n' "$body" | grep -Fq 'load_v256_from_ir1_mem_exact'
    printf '%s\n' "$body" | grep -Fq 'la_vreplgr2vr_d'
    printf '%s\n' "$body" | grep -Fq 'la_vreplvei_d'
    lsx_branch=$(printf '%s\n' "$body" | awk '
        /if \(1\)/ { in_branch = 1 }
        in_branch && /} else {/ { exit }
        in_branch { print }
    ')
    if printf '%s\n' "$lsx_branch" | grep -Eq '\bla_xv'; then
        echo "FAIL vmovddup LSX branch contains LASX" >&2
        exit 1
    fi
}

check_host_code()
{
    latx=$1
    dump=$(objdump -d "$latx" | awk '
        /<translate_vmovddup>:/ { in_function = 1 }
        in_function && /<translate_vmovddup>:/ { print; next }
        in_function && /^$/ { exit }
        in_function && /^[[:space:]]*[[:xdigit:]]+ <[^>]+>:/ { exit }
        in_function { print }
    ')
    if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>'; then
        echo "FAIL compiled vmovddup calls LASX generator" >&2
        exit 1
    fi
    printf '%s\n' "$dump" | grep -Fq '<la_vreplvei_d>'
    printf '%s\n' "$dump" | grep -Fq '<la_vreplgr2vr_d>'
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2 expected=$3
    mkdir -p "$expected"
    check_probe "$probe"
    "$probe" reference >"$expected/reference.bin"
    if [ "$(wc -c <"$expected/reference.bin")" -ne 1280 ]; then
        echo "FAIL reference output size" >&2
        exit 1
    fi
    normalise_reference "$expected/reference.bin" >"$expected/state.txt"
    if [ "$(wc -l <"$expected/state.txt")" -ne 40 ]; then
        echo "FAIL reference state count" >&2
        exit 1
    fi
    unique_states=$(sort -u "$expected/state.txt" | wc -l)
    if [ "$unique_states" -lt 10 ]; then
        echo "FAIL reference state diversity: $unique_states" >&2
        exit 1
    fi
    head -n 20 "$expected/state.txt" >"$expected/state-reg0.txt"
    tail -n 20 "$expected/state.txt" >"$expected/state-reg15.txt"
    for name in $fault_cases; do
        run_case '' "$probe" "$name" \
            "$expected/$name.bin" "$expected/$name.status" \
            "$expected/$name.stderr"
        check_fault_record "$expected/$name.bin" "$expected/$name.status"
    done
    echo "PASS generated x86 vmovddup reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2 probe=$3 expected=$4 source=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

    check_probe "$probe"
    marker=$(marker_pc "$probe")
    if [ -z "$marker" ]; then
        echo "FAIL missing vmovddup observer marker" >&2
        exit 1
    fi
    env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=0 \
        LATX_AVX_TRACE_YMM_INIT=1 \
        "$latx" "$probe" trace0 >"$tmpdir/trace0.stdout" \
        2>"$tmpdir/trace0.log"
    normalise_trace "$tmpdir/trace0.log" "$marker" \
        >"$tmpdir/state-reg0.txt"
    if [ "$(wc -l <"$tmpdir/state-reg0.txt")" -ne 20 ]; then
        echo "FAIL LATX YMM0 trace state count: $(wc -l <"$tmpdir/state-reg0.txt")" >&2
        exit 1
    fi
    cmp "$expected/state-reg0.txt" "$tmpdir/state-reg0.txt"

    env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
        LATX_AVX_TRACE_YMM_INIT=1 \
        "$latx" "$probe" trace15 >"$tmpdir/trace15.stdout" \
        2>"$tmpdir/trace15.log"
    normalise_trace "$tmpdir/trace15.log" "$marker" \
        >"$tmpdir/state-reg15.txt"
    if [ "$(wc -l <"$tmpdir/state-reg15.txt")" -ne 20 ]; then
        echo "FAIL LATX YMM15 trace state count: $(wc -l <"$tmpdir/state-reg15.txt")" >&2
        exit 1
    fi
    cmp "$expected/state-reg15.txt" "$tmpdir/state-reg15.txt"

    for name in $fault_cases; do
        run_case "$latx" "$probe" "$name" \
            "$tmpdir/$name.bin" "$tmpdir/$name.status" \
            "$tmpdir/$name.log"
        check_fault_record "$tmpdir/$name.bin" "$tmpdir/$name.status"
        cmp -n 32 "$expected/$name.bin" "$tmpdir/$name.bin"
        cmp "$expected/$name.status" "$tmpdir/$name.status"
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
    echo "PASS vmovddup XMM/YMM register, m64/m256, alias and fault differential"
    ;;
*) usage ;;
esac
