#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT_DIR]" >&2
    exit 2
fi
remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1903}
local_output_dir=${3:-/tmp/wi1903-x86-fixtures}
script_dir=$(cd "$(dirname "$0")" && pwd)
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi

for mnemonic in vmovhlps vmovlhps; do
    stem=latx-avx-single-$mnemonic
    for file in \
        "$script_dir/latx-avx-single-runtime.S" \
        "$script_dir/$stem.S" \
        "$script_dir/$stem.c" \
        "$script_dir/latx-avx-single-common.h" \
        "$script_dir/check-latx-avx-wi1903-mnemonic.sh"; do
        [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
    done
done

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-single-vmovhlps.S" \
    "$script_dir/latx-avx-single-vmovhlps.c" \
    "$script_dir/latx-avx-single-vmovlhps.S" \
    "$script_dir/latx-avx-single-vmovlhps.c" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1903-mnemonic.sh" \
    "$remote_host:$remote_dir/"

ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" <<'REMOTE'
set -euo pipefail
remote_dir=$1
docker run --rm --platform linux/amd64 -v "$remote_dir:/work" -w /work \
    latx-ci-baseline:latest bash -ceu '
        for mnemonic in vmovhlps vmovlhps; do
            stem=latx-avx-single-$mnemonic
            gcc -std=c11 -O0 -Wall -Wextra -Werror \
                -nostdlib -static -no-pie -ffreestanding -fno-builtin \
                -fno-stack-protector -mno-red-zone -mgeneral-regs-only \
                -mno-avx -mno-avx2 -I /work \
                -o "/work/$stem.static" \
                /work/latx-avx-single-runtime.S \
                "/work/$stem.S" "/work/$stem.c"
            chmod +x /work/check-latx-avx-wi1903-mnemonic.sh
            /work/check-latx-avx-wi1903-mnemonic.sh "/work/$stem.static" "$mnemonic"
            "/work/$stem.static" > "/work/$stem.native"
            test "$(wc -c < "/work/$stem.native")" -eq 128

            invalid=/tmp/$stem-invalid-memory.S
            printf ".text\\n%s %%xmm0, %%xmm1, (%%rax)\\n" "$mnemonic" > "$invalid"
            if gcc -c -mavx "$invalid" -o /tmp/$stem-invalid-memory.o \
                >/tmp/$stem-invalid-memory.stdout 2>/tmp/$stem-invalid-memory.stderr; then
                echo "FAIL $mnemonic accepted invalid memory form" >&2
                exit 1
            fi
        done
    '
REMOTE

mkdir -p "$local_output_dir"
for mnemonic in vmovhlps vmovlhps; do
    stem=latx-avx-single-$mnemonic
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output_dir/$stem.static"
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output_dir/$stem.static.native"
done
printf 'PASS WI-1903 xzy86 fixture build: output_dir=%s\n' "$local_output_dir"
