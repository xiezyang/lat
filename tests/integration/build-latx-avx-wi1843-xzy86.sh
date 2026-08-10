#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 4 ]]; then
    echo "usage: $0 MNEMONIC [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT]" >&2
    exit 2
fi
mnemonic=${1:?missing mnemonic}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1843-$mnemonic}
local_output=${4:-/tmp/latx-avx-wi1843-$mnemonic.static}
script_dir=$(cd "$(dirname "$0")" && pwd)
stem=latx-avx-single-$mnemonic
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
for file in \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/$stem.S" "$script_dir/$stem.c" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1843-mnemonic.sh"; do
    [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
done
ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/$stem.S" "$script_dir/$stem.c" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1843-mnemonic.sh" \
    "$remote_host:$remote_dir/"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$stem" "$mnemonic" <<'REMOTE'
set -euo pipefail
remote_dir=$1
stem=$2
mnemonic=$3
docker run --rm --platform linux/amd64 -e STEM="$stem" -e MNEMONIC="$mnemonic" \
    -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
    gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
        -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
        -mgeneral-regs-only -mno-avx -mno-avx2 -I /work \
        -o "/work/$STEM.static" /work/latx-avx-single-runtime.S \
        "/work/$STEM.S" "/work/$STEM.c"
    chmod +x /work/check-latx-avx-wi1843-mnemonic.sh
    /work/check-latx-avx-wi1843-mnemonic.sh "/work/$STEM.static" "$MNEMONIC"
    "/work/$STEM.static" > "/work/$STEM.native"
    test "$(wc -c < "/work/$STEM.native")" -eq 384
    invalid=/tmp/$STEM-invalid.S
    if [[ "$MNEMONIC" == vshufpd || "$MNEMONIC" == vshufps ]]; then
        bad_forms=(
            "$MNEMONIC xmm0, xmm1, xmm2"
            "$MNEMONIC xmmword ptr [rax], xmm1, xmm2, 0"
            "$MNEMONIC ymm0, xmm1, xmm2, 0"
            "$MNEMONIC xmm0, xmm1, ymmword ptr [rax], 0"
        )
    else
        bad_forms=(
            "$MNEMONIC xmm0, xmm1"
            "$MNEMONIC xmmword ptr [rax], xmm1, xmm2"
            "$MNEMONIC ymm0, xmm1, xmm2"
            "$MNEMONIC xmm0, xmm1, ymmword ptr [rax]"
        )
    fi
    for form in "${bad_forms[@]}"; do
        printf ".intel_syntax noprefix\n.text\n%s\n" "$form" > "$invalid"
        if gcc -c -mavx2 "$invalid" -o /tmp/$STEM-invalid.o \
            >/tmp/$STEM-invalid.stdout 2>/tmp/$STEM-invalid.stderr; then
            echo "FAIL accepted invalid form: $form" >&2
            exit 1
        fi
    done
    '
REMOTE
mkdir -p "$(dirname "$local_output")"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output.native"
printf 'PASS xzy86 WI-1843 fixture: mnemonic=%s output=%s\n' "$mnemonic" "$local_output"
