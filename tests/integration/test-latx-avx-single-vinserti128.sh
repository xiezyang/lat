#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

usage()
{
    echo "usage: $0 generate PROBE OUTPUT_DIR" >&2
    echo "       $0 verify LATX_X86_64 PROBE EXPECTED_DIR SOURCE" >&2
    exit 2
}

run_case()
{
    runner=$1 probe=$2 name=$3 output=$4 status_file=$5 error=$6
    set +e
    if [ -n "$runner" ]; then
        env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=15 \
            "$runner" "$probe" "$name" >"$output" 2>"$error"
    else
        "$probe" "$name" >"$output" 2>"$error"
    fi
    status=$?
    set -e
    printf '%s\n' "$status" >"$status_file"
}

check_probe()
{
    "$script_dir/check-latx-avx-single-mnemonic.sh" "$1" vinserti128
}

normalise_reference()
{
    od -An -v -tx8 -w32 "$1" | awk '{ print $1, $2, $3, $4 }'
}

normalise_trace()
{
    awk '
        /event=hit/ {
            observer = index($0, "bytes=c4430d38f6ff") != 0
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

merge_target_traces()
{
    trace15=$1 trace0=$2
    awk '
        NR == FNR { reg15[NR] = $0; next }
        {
            line_no = FNR
            if (line_no <= 256) {
                use15 = ((line_no - 1) % 10) < 5
            } else {
                use15 = ((line_no - 257) % 4) < 2
            }
            print use15 ? reg15[line_no] : $0
        }
    ' "$trace15" "$trace0"
}

check_fault_record()
{
    output=$1 status_file=$2
    if [ "$(sed -n '1p' "$status_file")" != 139 ]; then
        echo "FAIL fault-cross status: $(sed -n '1p' "$status_file")" >&2
        exit 1
    fi
    if [ "$(wc -c <"$output")" -ne 32 ]; then
        echo "FAIL fault-cross record size: $(wc -c <"$output")" >&2
        exit 1
    fi
    record_hex=$(od -An -tx1 "$output" | tr -d ' \n')
    case "$record_hex" in
        0b000000020000000010000000000000275e9dc08ab8136f50f8d6317c09a2e4) ;;
        *) echo "FAIL fault-cross record: $record_hex" >&2; exit 1 ;;
    esac
}

case "${1-}" in
generate)
    [ "$#" -eq 3 ] || usage
    probe=$2 expected=$3
    mkdir -p "$expected"
    check_probe "$probe"
    "$probe" reference >"$expected/reference.bin"
    if [ "$(wc -c <"$expected/reference.bin")" -ne 16384 ]; then
        echo "FAIL reference output size" >&2
        exit 1
    fi
    normalise_reference "$expected/reference.bin" >"$expected/state.txt"
    if [ "$(wc -l <"$expected/state.txt")" -ne 512 ]; then
        echo "FAIL reference state count" >&2
        exit 1
    fi
    run_case '' "$probe" fault-cross \
        "$expected/fault-cross.bin" "$expected/fault-cross.status" \
        "$expected/fault-cross.stderr"
    check_fault_record "$expected/fault-cross.bin" \
        "$expected/fault-cross.status"
    echo "PASS generated x86 vinserti128 reference"
    ;;
verify)
    [ "$#" -eq 5 ] || usage
    latx=$2 probe=$3 expected=$4 source=$5
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

    check_probe "$probe"
    for reg in 15 0; do
        env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM="$reg" \
            "$latx" "$probe" trace >"$tmpdir/trace-$reg.stdout" \
            2>"$tmpdir/trace-$reg.log"
        normalise_trace "$tmpdir/trace-$reg.log" \
            >"$tmpdir/state-$reg.txt"
        if [ "$(wc -l <"$tmpdir/state-$reg.txt")" -ne 512 ]; then
            echo "FAIL LATX trace state count for YMM$reg: $(wc -l <"$tmpdir/state-$reg.txt")" >&2
            exit 1
        fi
    done
    merge_target_traces "$tmpdir/state-15.txt" "$tmpdir/state-0.txt" \
        >"$tmpdir/state.txt"
    if [ "$(wc -l <"$tmpdir/state.txt")" -ne 512 ]; then
        echo "FAIL LATX trace state count: $(wc -l <"$tmpdir/state.txt")" >&2
        exit 1
    fi
    cmp "$expected/state.txt" "$tmpdir/state.txt"

    run_case "$latx" "$probe" fault-cross \
        "$tmpdir/fault-cross.bin" "$tmpdir/fault-cross.status" \
        "$tmpdir/fault-cross.log"
    check_fault_record "$tmpdir/fault-cross.bin" \
        "$tmpdir/fault-cross.status"
    cmp "$expected/fault-cross.bin" "$tmpdir/fault-cross.bin"
    cmp "$expected/fault-cross.status" "$tmpdir/fault-cross.status"
    last_state=$(grep -F 'event=ymm_state' "$tmpdir/fault-cross.log" |
        tail -n 1)
    case "$last_state" in
        *'reg=15 low0=6f13b88ac09d5e27 low1=e4a2097c31d6f850'*\
*'shadow_high0=1122334455667788 shadow_high1=99aabbccddeeff00'*) ;;
        *) echo "FAIL fault-cross changed YMM15: $last_state" >&2; exit 1 ;;
    esac

    body=$(awk '
        /bool translate_vinserti128\(/ { in_function = 1 }
        in_function { print }
        in_function && /^}/ { exit }
    ' "$source")
    printf '%s\n' "$body" | grep -Fq 'if (1)'
    printf '%s\n' "$body" | grep -Fq 'load_v128_from_ir1_mem_exact'
    lsx_branch=$(printf '%s\n' "$body" | awk '
        /if \(1\)/ { in_branch = 1 }
        in_branch && /} else {/ { exit }
        in_branch { print }
    ')
    if printf '%s\n' "$lsx_branch" | grep -Eq '\bla_xv'; then
        echo "FAIL vinserti128 LSX branch contains LASX" >&2
        exit 1
    fi
    dump=$(objdump -d --disassemble=translate_vinserti128 "$latx")
    if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>'; then
        echo "FAIL compiled vinserti128 calls LASX generator" >&2
        exit 1
    fi
    echo "PASS vinserti128 all imm8/forms, YMM state and cross-page fault"
    ;;
*) usage ;;
esac
