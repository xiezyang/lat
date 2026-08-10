#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "usage: $0 LATX_X86_64 FIXTURE_DIR OUTPUT_DIR" >&2
    exit 2
fi

latx=$1
fixture=$2
output=$3
script_dir=$(cd "$(dirname "$0")" && pwd)
expected_sha=${WI1875_BINARY_SHA256:-}

[[ -x "$latx" ]] || { echo "FAIL LATX binary is not executable" >&2; exit 2; }
[[ -n "$expected_sha" ]] || {
    echo "FAIL set WI1875_BINARY_SHA256 to the successful current build SHA-256" >&2
    exit 2
}
[[ -x "$fixture/vcvtsd2ss/latx-avx-single-vcvtsd2ss.static" ]] || {
    echo "FAIL VCVTSD2SS fixture missing" >&2
    exit 2
}
[[ -x "$fixture/vcvtsd2ss-daz/latx-avx-single-vcvtsd2ss-daz.static" ]] || {
    echo "FAIL VCVTSD2SS DAZ fixture missing" >&2
    exit 2
}

"$script_dir/test-latx-avx-wi1875-source.sh" "$(cd "$script_dir/../.." && pwd)"
mkdir -p "$output/vcvtsd2ss" "$output/vcvtsd2ss-daz"
binary_sha=$(sha256sum "$latx" | awk '{print $1}')
[[ "$binary_sha" == "$expected_sha" ]] || {
    echo "FAIL binary SHA-256 does not match WI1875_BINARY_SHA256" >&2
    exit 1
}
printf 'binary=%s\nbinary_sha256=%s\n' "$latx" "$binary_sha" > "$output/run-metadata.txt"

signal_status() {
    local log=$1
    local signal
    signal=$(sed -n 's/.*received signal \(SIG[A-Z0-9]*\).*/\1/p' "$log" | tail -1)
    case "$signal" in
        SIGFPE) printf '136' ;;
        SIGSEGV) printf '139' ;;
        SIGBUS) printf '138' ;;
        SIGILL) printf '132' ;;
        '')
            local code
            code=$(sed -n 's/.*exited with code \([0-9][0-9]*\).*/\1/p' "$log" | tail -1)
            if [[ -n "$code" ]]; then
                code=${code##0}
                [[ -n "$code" ]] || code=0
                printf '%s' "$((8#$code))"
            elif grep -q 'exited normally' "$log"; then
                printf '0'
            else
                printf 'unknown'
            fi
            ;;
        *) printf 'unknown' ;;
    esac
}

run_lasx() {
    local family=$1 name=$2 probe=$3
    local dir="$output/$family"
    set +e
    LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" "$name" \
        > "$dir/lasx-$name.bin" 2> "$dir/lasx-$name.stderr"
    rc=$?
    set -e
    printf '%s\n' "$rc" > "$dir/lasx-$name.status"
}

run_lsx() {
    local family=$1 name=$2 probe=$3
    local dir="$output/$family"
    local addr
    addr=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1; exit }')
    [[ -n "$addr" ]] || { echo "FAIL option_enable_lasx symbol missing" >&2; exit 1; }
    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'set confirm off' \
        -ex 'handle SIGFPE pass nostop noprint' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex 'break translate_context_init' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        -ex "run $probe $name > $dir/lsx-$name.bin 2> $dir/lsx-$name.stderr" \
        -ex "set {int}$addr = 0" \
        -ex 'printf "option_enable_lasx_readback=%d\\n", *(int *)&option_enable_lasx' \
        -ex continue > "$dir/lsx-$name.gdb.log" 2>&1
    gdb_rc=$?
    set -e
    grep -Fq 'option_enable_lasx_readback=0' "$dir/lsx-$name.gdb.log" || {
        echo "FAIL LSX option_enable_lasx readback: $family/$name" >&2
        exit 1
    }
    printf '%s\n' "$gdb_rc" > "$dir/lsx-$name.gdb.status"
    signal_status "$dir/lsx-$name.gdb.log" > "$dir/lsx-$name.status"
}

run_family() {
    local family=$1 probe=$2
    local expected_dir="$fixture/$family"
    local names=(reference)
    [[ "$family" == vcvtsd2ss ]] && names+=(fpe-invalid fpe-precision fpe-overflow fpe-underflow)
    for name in "${names[@]}"; do
        run_lasx "$family" "$name" "$probe"
        run_lsx "$family" "$name" "$probe"
        cmp "$expected_dir/$name.bin" "$output/$family/lsx-$name.bin"
        cmp "$expected_dir/$name.status" "$output/$family/lsx-$name.status"
        if ! cmp -s "$expected_dir/$name.bin" "$output/$family/lasx-$name.bin" ||
            ! cmp -s "$expected_dir/$name.status" "$output/$family/lasx-$name.status"; then
            printf 'INFO LASX deviation: %s/%s\n' "$family" "$name"
        fi
    done
}

run_family vcvtsd2ss "$fixture/vcvtsd2ss/latx-avx-single-vcvtsd2ss.static"
run_family vcvtsd2ss-daz "$fixture/vcvtsd2ss-daz/latx-avx-single-vcvtsd2ss-daz.static"
sha256sum "$latx" "$script_dir/latx-avx-single-vcvtsd2ss.S" \
    "$script_dir/latx-avx-single-vcvtsd2ss.c" \
    "$script_dir/latx-avx-single-vcvtsd2ss-daz.S" \
    "$script_dir/latx-avx-single-vcvtsd2ss-daz.c" \
    > "$output/source-binary.sha256"
echo 'PASS WI-1875 VCVTSD2SS x86/LASX/forced-LSX comparison'
