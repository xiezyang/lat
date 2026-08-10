#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT]" >&2
    exit 2
fi
remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1907-vmovntdqa}
local_output=${3:-/tmp/latx-avx-wi1907-vmovntdqa.static}
script_dir=$(cd "$(dirname "$0")" && pwd)
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
stem=latx-avx-single-vmovntdqa
for file in \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/$stem.S" "$script_dir/$stem.c" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1907-mnemonic.sh"; do
    [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
done
ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" "$script_dir/$stem.S" \
    "$script_dir/$stem.c" "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1907-mnemonic.sh" \
    "$remote_host:$remote_dir/"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$stem" <<'REMOTE'
set -euo pipefail
remote_dir=$1
stem=$2
docker run --rm --platform linux/amd64 -e STEM="$stem" \
    -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
    gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
        -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
        -mgeneral-regs-only -mno-avx -mno-avx2 -I /work \
        -o "/work/$STEM.static" /work/latx-avx-single-runtime.S \
        "/work/$STEM.S" "/work/$STEM.c"
    chmod +x /work/check-latx-avx-wi1907-mnemonic.sh
    /work/check-latx-avx-wi1907-mnemonic.sh "/work/$STEM.static" vmovntdqa
    "/work/$STEM.static" > "/work/$STEM.native"
    test "$(wc -c < "/work/$STEM.native")" -eq 128
    for case_name in xmm-unaligned ymm-unaligned xmm-cross ymm-cross; do
        if "/work/$STEM.static" "$case_name"; then
            echo "FAIL $case_name must fault" >&2
            exit 1
        else
            test "$?" -eq 139
        fi
    done
    invalid=/tmp/$STEM-invalid-register.S
    for form in \
        "vmovntdqa xmm0, xmm1" \
        "vmovntdqa ymm0, ymm1"; do
        printf ".intel_syntax noprefix\\n.text\\n%s\\n" "$form" > "$invalid"
        if gcc -c -mavx2 "$invalid" -o /tmp/$STEM-invalid-register.o \
            >/tmp/$STEM-invalid-register.stdout 2>/tmp/$STEM-invalid-register.stderr; then
            echo "FAIL accepted invalid register-source form: $form" >&2
            exit 1
        fi
    done
    '
REMOTE
mkdir -p "$(dirname "$local_output")"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output.native"
printf 'PASS WI-1907 xzy86 fixture: output=%s\n' "$local_output"
