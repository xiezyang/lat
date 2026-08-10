#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
    echo "usage: $0 LATX_BINARY X86_FIXTURE X86_ARTIFACT_DIR OUTPUT_DIR" >&2
    exit 2
fi

latx=$1
fixture=$2
expected_dir=$3
output_dir=$4
option_address=$(nm -g "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1; exit }')
[[ -n "$option_address" ]] || { echo "FAIL option_enable_lasx symbol missing" >&2; exit 1; }
mkdir -p "$output_dir"

sha256() { sha256sum "$1" | awk '{print $1}'; }

run_x86()
{
    local name=$1
    local dir="$output_dir/$name"
    mkdir -p "$dir"
    if [[ "$name" == normal ]]; then
        cp "$expected_dir/latx-avx-single-vmaskmovdqu.static.native" "$dir/x86.stdout"
        : >"$dir/x86.stderr"
        printf '0\n' >"$dir/x86.status"
    else
        cp "$expected_dir/latx-avx-single-vmaskmovdqu.static.$name.stdout" "$dir/x86.stdout"
        : >"$dir/x86.stderr"
        cp "$expected_dir/latx-avx-single-vmaskmovdqu.static.$name.status" "$dir/x86.status"
    fi
}

run_lasx()
{
    local name=$1
    shift
    local dir="$output_dir/$name"
    set +e
    env LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$fixture" "$@" \
        >"$dir/lasx.stdout" 2>"$dir/lasx.stderr"
    local rc=$?
    set -e
    printf '%s\n' "$rc" >"$dir/lasx.status"
}

run_lsx()
{
    local name=$1
    shift
    local dir="$output_dir/$name"
    local guest_cmd="$fixture"
    local arg
    for arg in "$@"; do
        guest_cmd+=" $arg"
    done
    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'set confirm off' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        -ex 'break translate_context_init' \
        -ex "run $guest_cmd > $dir/lsx.stdout 2> $dir/lsx.stderr" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue \
        >"$dir/lsx.gdb" 2>&1
    local gdb_rc=$?
    set -e
    python3 - "$dir/lsx.gdb" "$gdb_rc" >"$dir/lsx.status" <<'PY'
import re
import sys

text = open(sys.argv[1], encoding="utf-8", errors="replace").read()
fallback = int(sys.argv[2])
if "exited normally" in text:
    print(0)
    raise SystemExit
match = re.search(r"exited with code ([0-7]+)", text)
if match:
    print(int(match.group(1), 8))
    raise SystemExit
signals = {"SIGSEGV": 11, "SIGBUS": 7, "SIGILL": 4, "SIGFPE": 8}
match = re.search(r"received signal (SIG[A-Z0-9]+)", text)
if match and match.group(1) in signals:
    print(128 + signals[match.group(1)])
else:
    print(fallback)
PY
    grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$dir/lsx.gdb"
}

run_x86 normal
run_lasx normal
run_lsx normal
for case_name in store-zero store-one store-mix; do
    run_x86 "$case_name" "$case_name"
    run_lasx "$case_name" "$case_name"
    run_lsx "$case_name" "$case_name"
done

printf 'case\tx86_rc\tlasx_rc\tlsx_rc\toption_enable_lasx\tx86_sha256\tlasx_sha256\tlsx_sha256\tlsx_byte_equal\tlasx_byte_equal\tx86_binary_sha256\tfixture_sha256\n' \
    >"$output_dir/results.tsv"
failed=0
for case_name in normal store-zero store-one store-mix; do
    dir="$output_dir/$case_name"
    x86_rc=$(cat "$dir/x86.status")
    lasx_rc=$(cat "$dir/lasx.status")
    lsx_rc=$(cat "$dir/lsx.status")
    x86_hash=$(sha256 "$dir/x86.stdout")
    lasx_hash=$(sha256 "$dir/lasx.stdout")
    lsx_hash=$(sha256 "$dir/lsx.stdout")
    x86_equal=FAIL
    lasx_equal=FAIL
    cmp -s "$dir/x86.stdout" "$dir/lsx.stdout" && x86_equal=PASS
    cmp -s "$dir/x86.stdout" "$dir/lasx.stdout" && lasx_equal=PASS
    if [[ "$x86_equal" != PASS || "$x86_rc" != "$lsx_rc" || "$lsx_rc" != 0 && "$case_name" == normal ]]; then
        failed=1
    fi
    printf '%s\t%s\t%s\t%s\t0\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$case_name" "$x86_rc" "$lasx_rc" "$lsx_rc" "$x86_hash" \
        "$lasx_hash" "$lsx_hash" "$x86_equal" "$lasx_equal" \
        "$(sha256 "$latx")" "$(sha256 "$fixture")" >>"$output_dir/results.tsv"
    printf '%s: x86=%s LASX=%s LSX=%s option=0 LSX-bytes=%s LASX-bytes=%s\n' \
        "$case_name" "$x86_rc" "$lasx_rc" "$lsx_rc" "$x86_equal" "$lasx_equal"
done

if [[ "$failed" -ne 0 ]]; then
    echo "FAIL WI-1899 LSX comparison" >&2
    exit 1
fi
echo "PASS WI-1899 x86/LASX/LSX comparison"
