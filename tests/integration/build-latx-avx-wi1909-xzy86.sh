#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT_DIR]" >&2
    exit 2
fi
remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1909}
local_output_dir=${3:-/tmp/wi1909-x86-fixtures}
script_dir=$(cd "$(dirname "$0")" && pwd)
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
for mnemonic in vmovshdup vmovsldup; do
    stem=latx-avx-single-$mnemonic
    for file in "$script_dir/latx-avx-single-runtime.S" \
        "$script_dir/$stem.S" "$script_dir/$stem.c" \
        "$script_dir/latx-avx-single-common.h" \
        "$script_dir/check-latx-avx-wi1909-mnemonic.sh"; do
        [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
    done
done
ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-single-vmovshdup.S" "$script_dir/latx-avx-single-vmovshdup.c" \
    "$script_dir/latx-avx-single-vmovsldup.S" "$script_dir/latx-avx-single-vmovsldup.c" \
    "$script_dir/latx-avx-single-common.h" "$script_dir/check-latx-avx-wi1909-mnemonic.sh" \
    "$remote_host:$remote_dir/"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" <<'REMOTE'
set -euo pipefail
remote_dir=$1
docker run --rm --platform linux/amd64 -v "$remote_dir:/work" -w /work \
    latx-ci-baseline:latest bash -ceu '
    for mnemonic in vmovshdup vmovsldup; do
        stem=latx-avx-single-$mnemonic
        gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
            -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
            -mgeneral-regs-only -mno-avx -mno-avx2 -I /work \
            -o "/work/$stem.static" /work/latx-avx-single-runtime.S \
            "/work/$stem.S" "/work/$stem.c"
        chmod +x /work/check-latx-avx-wi1909-mnemonic.sh
        /work/check-latx-avx-wi1909-mnemonic.sh "/work/$stem.static" "$mnemonic"
        "/work/$stem.static" > "/work/$stem.native"
        test "$(wc -c < "/work/$stem.native")" -eq 192
        for case_name in xmm-cross ymm-cross; do
            if "/work/$stem.static" "$case_name"; then
                echo "FAIL $mnemonic $case_name must fault" >&2
                exit 1
            else
                test "$?" -eq 139
            fi
        done
        invalid=/tmp/$stem-invalid.S
        for form in \
            "$mnemonic xmm0, xmm1, xmm2" \
            "$mnemonic dword ptr [rax], xmm0" \
            "$mnemonic xmm0, ymm0"; do
            printf ".intel_syntax noprefix\\n.text\\n%s\\n" "$form" > "$invalid"
            if gcc -c -mavx2 "$invalid" -o /tmp/$stem-invalid.o \
                >/tmp/$stem-invalid.stdout 2>/tmp/$stem-invalid.stderr; then
                echo "FAIL accepted invalid form: $form" >&2
                exit 1
            fi
        done
    done
    '
REMOTE
mkdir -p "$local_output_dir"
for mnemonic in vmovshdup vmovsldup; do
    stem=latx-avx-single-$mnemonic
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output_dir/$stem.static"
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output_dir/$stem.static.native"
done
printf 'PASS WI-1909 xzy86 fixture build: output_dir=%s\n' "$local_output_dir"
