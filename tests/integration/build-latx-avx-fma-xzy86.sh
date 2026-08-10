#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 4 ]]; then
    echo "usage: $0 MNEMONIC [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT]" >&2
    exit 2
fi
mnemonic=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-fma-$mnemonic}
local_output=${4:-/tmp/latx-avx-single-$mnemonic.static}
script_dir=$(cd "$(dirname "$0")" && pwd)
stem="latx-avx-single-$mnemonic"
ssh_args=(); scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG"); scp_args=(-F "$LATX_SSH_CONFIG")
fi
ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/$stem.S" "$script_dir/$stem.c" \
    "$script_dir/latx-avx-single-common.h" "$remote_host:$remote_dir/"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$stem" "$mnemonic" <<'REMOTE'
set -euo pipefail
remote_dir=$1; stem=$2; mnemonic=$3
docker run --rm -e STEM="$stem" -e MNEMONIC="$mnemonic" \
    -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
    gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
        -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
        -mgeneral-regs-only -mno-avx -mno-avx2 -I /work -o "/work/$STEM.static" \
        /work/latx-avx-single-runtime.S "/work/$STEM.S" "/work/$STEM.c"
    found=$(objdump -d --no-show-raw-insn -Mintel "/work/$STEM.static" |
        awk '\''$2 ~ /^v[a-zA-Z0-9_]+$/ { print $2 }'\'' | sort -u)
    printf "%s\\n" "$found" | grep -Fxq "$MNEMONIC"
    chmod +x "/work/$STEM.static"
    "/work/$STEM.static" > "/work/$STEM.native"
'
REMOTE
mkdir -p "$(dirname "$local_output")"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output.native"
printf 'PASS xzy86 FMA static build: mnemonic=%s output=%s\n' "$mnemonic" "$local_output"
