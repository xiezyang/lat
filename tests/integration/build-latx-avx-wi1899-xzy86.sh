#!/usr/bin/env bash
set -euo pipefail
if [[ $# -gt 3 ]]; then echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT]" >&2; exit 2; fi
remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1899-vmaskmovdqu}
local_output=${3:-/tmp/latx-avx-wi1899-vmaskmovdqu.static}
script_dir=$(cd "$(dirname "$0")" && pwd)
stem=latx-avx-single-vmaskmovdqu
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then ssh_args=(-F "$LATX_SSH_CONFIG"); scp_args=(-F "$LATX_SSH_CONFIG"); fi
for file in "$script_dir/latx-avx-single-runtime.S" "$script_dir/$stem.S" "$script_dir/$stem.c" "$script_dir/latx-avx-single-common.h" "$script_dir/check-latx-avx-wi1899-mnemonic.sh"; do
    [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
done
ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$script_dir/latx-avx-single-runtime.S" "$script_dir/$stem.S" "$script_dir/$stem.c" "$script_dir/latx-avx-single-common.h" "$script_dir/check-latx-avx-wi1899-mnemonic.sh" "$remote_host:$remote_dir/"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$stem" <<'REMOTE'
set -euo pipefail
remote_dir=$1
stem=$2
docker run --rm --platform linux/amd64 -e STEM="$stem" -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
    gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone -mgeneral-regs-only -mno-avx -mno-avx2 -I /work -o "/work/$STEM.static" /work/latx-avx-single-runtime.S "/work/$STEM.S" "/work/$STEM.c"
    chmod +x /work/check-latx-avx-wi1899-mnemonic.sh
    /work/check-latx-avx-wi1899-mnemonic.sh "/work/$STEM.static" vmaskmovdqu
    "/work/$STEM.static" > "/work/$STEM.native"
    for case_name in store-zero store-one store-mix; do
        if "/work/$STEM.static" "$case_name" > "/work/$STEM.$case_name.stdout" 2> "/work/$STEM.$case_name.stderr"; then
            rc=0
        else
            rc=$?
        fi
        echo "$rc" > "/work/$STEM.$case_name.status"
        test "$rc" -eq 139
    done
    '
REMOTE
mkdir -p "$(dirname "$local_output")"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output.native"
for case_name in store-zero store-one store-mix; do
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.$case_name.stdout" "$local_output.$case_name.stdout"
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.$case_name.status" "$local_output.$case_name.status"
done
printf 'PASS xzy86 WI-1899 fixture: output=%s\n' "$local_output"
