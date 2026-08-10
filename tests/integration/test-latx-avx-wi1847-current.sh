#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "usage: $0 LATX_X86_64 X86_FIXTURE_DIR OUTPUT_DIR" >&2
    exit 2
fi

latx=$1
x86_dir=$2
output=$3
expected_binary_sha256=${WI1847_BINARY_SHA256:-}
script_dir=$(cd "$(dirname "$0")" && pwd)
repo_dir=$(cd "$script_dir/../.." && pwd)

[[ -x "$latx" ]] || { echo "FAIL LATX binary is not executable: $latx" >&2; exit 2; }
[[ -d "$x86_dir" ]] || { echo "FAIL xzy86 fixture directory is missing: $x86_dir" >&2; exit 2; }
[[ -n "$expected_binary_sha256" ]] || {
    echo "FAIL set WI1847_BINARY_SHA256 to the SHA-256 from the successful current build" >&2
    exit 2
}
command -v gdb >/dev/null 2>&1 || { echo "FAIL gdb is required" >&2; exit 2; }
command -v nm >/dev/null 2>&1 || { echo "FAIL nm is required" >&2; exit 2; }

mkdir -p "$output"
binary_sha256=$(sha256sum "$latx" | awk '{print $1}')
[[ "$binary_sha256" == "$expected_binary_sha256" ]] || {
    echo "FAIL binary SHA-256 is not the approved current build" >&2
    exit 1
}
option_address=$(nm -g "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1; exit }')
[[ -n "$option_address" ]] || { echo "FAIL option_enable_lasx symbol is missing" >&2; exit 1; }

printf 'binary=%s\nbinary_sha256=%s\noption_enable_lasx_address=%s\n' \
    "$latx" "$binary_sha256" "$option_address" > "$output/run-metadata.txt"
printf 'source_dir=%s\n' "$repo_dir/tests/integration" >> "$output/run-metadata.txt"

signal_number() {
    case "$1" in
        SIGILL) printf '4' ;;
        SIGFPE) printf '8' ;;
        SIGSEGV) printf '11' ;;
        SIGBUS) printf '7' ;;
        *) printf '0' ;;
    esac
}

gdb_status() {
    local log=$1
    local signal
    signal=$(sed -n \
        -e 's/.*received signal \(SIG[A-Z0-9]*\).*/\1/p' \
        -e 's/.*terminated with signal \(SIG[A-Z0-9]*\).*/\1/p' \
        "$log" | tail -1)
    if [[ -n "$signal" ]]; then
        printf '%s' "$((128 + $(signal_number "$signal")))"
    elif grep -q 'exited normally' "$log"; then
        printf '0'
    else
        local code
        code=$(sed -n 's/.*exited with code \([0-9][0-9]*\).*/\1/p' "$log" | tail -1)
        if [[ -n "$code" ]]; then
            code=${code##0}
            [[ -n "$code" ]] || code=0
            printf '%s' "$((8#$code))"
        else
            printf 'unknown'
        fi
    fi
}

run_lasx() {
    local name=$1
    shift
    local guest_args=("$@")
    set +e
    env LATX_AVX_CPUID="${LATX_AVX_CPUID:-0}" LATX_AOT=0 \
        "$latx" "$x86_dir/latx-avx-cpuid-probe.static" "${guest_args[@]}" \
        > "$output/lasx-$name.stdout" 2> "$output/lasx-$name.stderr"
    printf '%s\n' "$?" > "$output/lasx-$name.status"
    set -e
}

run_lsx() {
    local name=$1
    local cpuid=$2
    local arg=$3
    local command_line="run $x86_dir/latx-avx-cpuid-probe.static $arg > $output/lsx-$name.stdout 2> $output/lsx-$name.stderr"
    {
        printf 'gdb -q -batch %s\n' "$latx"
        printf '  set environment LATX_AVX_CPUID %s\n' "$cpuid"
        printf '  break translate_context_init\n'
        printf '  %s\n' "$command_line"
        printf '  set {int}%s = 0\n  x/wd %s\n  continue\n' "$option_address" "$option_address"
    } > "$output/lsx-$name.commands"
    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'set confirm off' \
        -ex 'handle SIGILL pass nostop noprint' \
        -ex 'handle SIGFPE pass nostop noprint' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex "set environment LATX_AVX_CPUID $cpuid" \
        -ex 'set environment LATX_AOT 0' \
        -ex 'break translate_context_init' \
        -ex "$command_line" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue > "$output/lsx-$name.gdb" 2>&1
    gdb_rc=$?
    set -e
    grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$output/lsx-$name.gdb" || {
        echo "FAIL LSX option_enable_lasx readback for $name" >&2
        exit 1
    }
    printf '%s\n' "$gdb_rc" > "$output/lsx-$name.gdb.status"
    gdb_status "$output/lsx-$name.gdb" > "$output/lsx-$name.status"
    [[ -f "$output/lsx-$name.stdout" ]] || : > "$output/lsx-$name.stdout"
    [[ -f "$output/lsx-$name.stderr" ]] || : > "$output/lsx-$name.stderr"
}

run_lsx hidden-info 0 info
run_lsx hidden-xgetbv 0 xgetbv
run_lsx hidden-guarded 0 guarded
run_lsx hidden-unconditional 0 unconditional
run_lsx enabled-unconditional 1 unconditional
run_lsx hidden-bmi2 0 bmi2

grep -Fq 'fma=0 xsave=0 osxsave=0 avx=0 f16c=0' \
    "$output/lsx-hidden-info.stdout"
grep -Fq 'bmi1=0' "$output/lsx-hidden-info.stdout"
grep -Fq 'bmi2=0' "$output/lsx-hidden-info.stdout"
grep -Fq 'avx2=0' "$output/lsx-hidden-info.stdout"
grep -Fq 'avx512f=0' "$output/lsx-hidden-info.stdout"

[[ "$(cat "$output/lsx-hidden-xgetbv.status")" == 132 ]]
[[ "$(cat "$output/lsx-hidden-unconditional.status")" == 132 ]]
[[ "$(cat "$output/lsx-hidden-guarded.status")" == 0 ]]
[[ "$(cat "$output/lsx-enabled-unconditional.status")" == 0 ]]
[[ "$(cat "$output/lsx-hidden-bmi2.status")" == 0 ]]

[[ "$(cat "$x86_dir/x86-xgetbv.status")" == 0 ]]
[[ "$(cat "$x86_dir/x86-guarded.status")" == 0 ]]
[[ "$(cat "$x86_dir/x86-unconditional.status")" == 0 ]]
[[ "$(cat "$x86_dir/x86-bmi2.status")" == 0 ]]

# LASX is diagnostic only: its result is recorded and never decides LSX pass.
for name in hidden-info hidden-xgetbv hidden-guarded hidden-unconditional \
    enabled-unconditional hidden-bmi2; do
    arg=${name#*-}
    cpuid=0
    [[ "$name" == enabled-* ]] && cpuid=1
    LATX_AVX_CPUID=$cpuid run_lasx "$name" "$arg"
done

sha256sum "$latx" \
    "$script_dir/latx-avx-cpuid-probe.c" \
    "$script_dir/latx-avx-cpuid-probe.S" \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/test-latx-avx-wi1847-current.sh" \
    > "$output/source-binary.sha256"
printf 'PASS prepared WI-1847 run set; no result is accepted without current binary SHA and xzy86 fixture hashes\n'
