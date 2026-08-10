#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 1 || $# -gt 4 ]]; then
    echo "usage: $0 LATX_X86_64 [REMOTE_HOST] [ROUNDS] [REMOTE_DIR]" >&2
    exit 2
fi

latx=$(realpath "$1")
remote_host=${2:-xzy86}
rounds=${3:-1024}
remote_dir=${4:-/tmp/latx-avx-random-diff}
source_dir=$(cd "$(dirname "$0")" && pwd)
local_dir=$(mktemp -d)
tests=(vmovdqa vmovdqu vpxor vextracti128 vzeroupper vmovss)

cleanup()
{
    rm -rf "$local_dir"
}
trap cleanup EXIT HUP INT TERM

if [[ ! -x "$latx" ]]; then
    echo "FAIL LATX binary is not executable: $latx" >&2
    exit 2
fi
if ! [[ "$rounds" =~ ^[1-9][0-9]*$ ]] || (( rounds > 100000 )); then
    echo "FAIL rounds must be between 1 and 100000" >&2
    exit 2
fi

ssh "$remote_host" mkdir -p "$remote_dir"
scp "$source_dir"/latx-avx-random-common.h \
    "$source_dir"/latx-avx-random-*.c \
    "$source_dir"/latx-avx-random-*.S \
    "$remote_host:$remote_dir/"

ssh "$remote_host" bash -s -- "$remote_dir" "$rounds" <<'REMOTE'
set -euo pipefail

remote_dir=$1
rounds=$2
tests=(vmovdqa vmovdqu vpxor vextracti128 vzeroupper vmovss)

docker run --rm -v "$remote_dir:/work" -w /work latx-ci-baseline:latest \
    bash -ceu '
        for test in vmovdqa vmovdqu vpxor vextracti128 vzeroupper vmovss; do
            gcc -std=c11 -O0 -Wall -Wextra -Werror -fno-tree-vectorize \
                -fno-tree-slp-vectorize -static -no-pie \
                -o "latx-avx-random-${test}.static" \
                "latx-avx-random-${test}.c" "latx-avx-random-${test}.S"
        done
    '

cd "$remote_dir"
for test in "${tests[@]}"; do
    "./latx-avx-random-${test}.static" "$rounds" \
        >"latx-avx-random-${test}.native"
done
REMOTE

mkdir "$local_dir/remote"
scp -r "$remote_host:$remote_dir/." "$local_dir/remote/"

for test in "${tests[@]}"; do
    native_output="$local_dir/remote/latx-avx-random-${test}.native"
    latx_output="$local_dir/latx-avx-random-${test}.latx"
    stderr_output="$local_dir/latx-avx-random-${test}.stderr"

    env LATX_AVX_CPUID=0 "$latx" \
        "$local_dir/remote/latx-avx-random-${test}.static" "$rounds" \
        >"$latx_output" 2>"$stderr_output"

    if ! cmp -s "$native_output" "$latx_output"; then
        echo "FAIL $test: x86 and LATX output differ" >&2
        cmp -l "$native_output" "$latx_output" | head -20 >&2 || true
        echo "native bytes: $(wc -c <"$native_output")" >&2
        echo "LATX bytes: $(wc -c <"$latx_output")" >&2
        if [[ -s "$stderr_output" ]]; then
            sed -n '1,80p' "$stderr_output" >&2
        fi
        exit 1
    fi

    printf 'PASS %s rounds=%s bytes=%s sha256=%s\n' "$test" "$rounds" \
        "$(wc -c <"$native_output")" \
        "$(sha256sum "$native_output" | awk '{print $1}')"
done

printf 'PASS all six random differential tests; x86 source and binaries: %s:%s\n' \
    "$remote_host" "$remote_dir"
