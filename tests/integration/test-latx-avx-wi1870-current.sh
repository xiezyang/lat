#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "usage: $0 LATX_X86_64 FIXTURE_DIR OUTPUT_DIR" >&2
    exit 2
fi

latx=$1
fixture=$2
output=$3
expected_sha=${WI1870_BINARY_SHA256:-}
script_dir=$(cd "$(dirname "$0")" && pwd)
[[ -x "$latx" ]] || { echo "FAIL LATX binary is not executable" >&2; exit 2; }
[[ -n "$expected_sha" ]] || {
    echo "FAIL set WI1870_BINARY_SHA256 to the successful current build SHA-256" >&2
    exit 2
}
for item in vcomisd vcomiss vucomiss; do
    [[ -x "$fixture/$item/latx-avx-$item.static" ]] || {
        echo "FAIL missing fixture: $item" >&2
        exit 2
    }
done

"$script_dir/test-latx-avx-wi1870-source.sh" "$(cd "$script_dir/../.." && pwd)"
binary_sha=$(sha256sum "$latx" | awk '{print $1}')
[[ "$binary_sha" == "$expected_sha" ]] || {
    echo "FAIL binary SHA-256 does not match WI1870_BINARY_SHA256" >&2
    exit 1
}
mkdir -p "$output"
printf 'binary=%s\nbinary_sha256=%s\n' "$latx" "$binary_sha" > "$output/run-metadata.txt"

inferior_status() {
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

for item in vcomisd vcomiss vucomiss; do
    mkdir -p "$output/$item"
    probe="$fixture/$item/latx-avx-$item.static"
    for name in reference fpe-invalid fpe-denormal; do
        set +e
        LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" "$name" \
            > "$output/$item/lasx-$name.bin" 2> "$output/$item/lasx-$name.stderr"
        printf '%s\n' "$?" > "$output/$item/lasx-$name.status"
        set -e
        addr=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1; exit }')
        gdb -q -batch "$latx" \
            -ex 'set pagination off' \
            -ex 'set confirm off' \
            -ex 'handle SIGFPE pass nostop noprint' \
            -ex 'break translate_context_init' \
            -ex 'set environment LATX_AVX_CPUID 1' \
            -ex 'set environment LATX_AOT 0' \
            -ex "run $probe $name > $output/$item/lsx-$name.bin 2> $output/$item/lsx-$name.stderr" \
            -ex "set {int}$addr = 0" \
            -ex 'printf "option_enable_lasx_readback=%d\\n", *(int *)&option_enable_lasx' \
            -ex continue > "$output/$item/lsx-$name.gdb.log" 2>&1 || true
        grep -Fq 'option_enable_lasx_readback=0' "$output/$item/lsx-$name.gdb.log"
        inferior_status "$output/$item/lsx-$name.gdb.log" \
            > "$output/$item/lsx-$name.status"
        cp "$fixture/$item/$name.bin" "$output/$item/x86-$name.bin"
        cp "$fixture/$item/$name.status" "$output/$item/x86-$name.status"
        cmp "$output/$item/x86-$name.bin" "$output/$item/lsx-$name.bin"
        cmp "$output/$item/x86-$name.status" "$output/$item/lsx-$name.status"
        if ! cmp -s "$output/$item/x86-$name.bin" "$output/$item/lasx-$name.bin" ||
            ! cmp -s "$output/$item/x86-$name.status" "$output/$item/lasx-$name.status"; then
            printf 'INFO LASX deviation: %s/%s\n' "$item" "$name"
        fi
    done
done
sha256sum "$latx" "$script_dir/latx-avx-wi1870-comis.S" \
    "$script_dir/latx-avx-wi1870-comis.c" \
    "$script_dir/wi1870-comis-fixtures.json" \
    > "$output/source-binary.sha256"
echo 'PASS WI-1870 VCOMISD/VCOMISS/VUCOMISS x86/LASX/forced-LSX comparison'
