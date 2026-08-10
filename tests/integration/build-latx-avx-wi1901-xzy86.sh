#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT]" >&2
    exit 2
fi
remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1901-vmovddup}
local_output=${3:-/tmp/latx-avx-wi1901-vmovddup.static}
script_dir=$(cd "$(dirname "$0")" && pwd)
stem=latx-avx-single-vmovddup
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi

for file in \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/$stem.S" \
    "$script_dir/$stem.c" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-single-mnemonic.sh"; do
    [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
done

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/$stem.S" \
    "$script_dir/$stem.c" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-single-mnemonic.sh" \
    "$remote_host:$remote_dir/"

ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$stem" <<'REMOTE'
set -euo pipefail
remote_dir=$1
stem=$2
docker run --rm --platform linux/amd64 \
    -e STEM="$stem" \
    -v "$remote_dir:/work" -w /work latx-ci-baseline:latest \
    bash -ceu '
        gcc -std=c11 -O0 -Wall -Wextra -Werror \
            -nostdlib -static -no-pie -ffreestanding -fno-builtin \
            -fno-stack-protector -mno-red-zone -mgeneral-regs-only \
            -mno-avx -mno-avx2 -I /work \
            -o "/work/$STEM.static" \
            /work/latx-avx-single-runtime.S \
            "/work/$STEM.S" \
            "/work/$STEM.c"
        chmod +x /work/check-latx-avx-single-mnemonic.sh
        /work/check-latx-avx-single-mnemonic.sh "/work/$STEM.static" vmovddup
        "/work/$STEM.static" > "/work/$STEM.native"
        test "$(wc -c < "/work/$STEM.native")" -eq 1280
    '
REMOTE

mkdir -p "$(dirname "$local_output")"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output.native"
printf 'PASS xzy86 WI-1901 fixture build: output=%s\n' "$local_output"
