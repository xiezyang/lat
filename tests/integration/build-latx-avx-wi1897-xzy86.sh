#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 4 ]]; then
    echo "usage: $0 MNEMONIC REMOTE_HOST [REMOTE_DIR] [LOCAL_OUTPUT]" >&2
    exit 2
fi
mnemonic=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')
case "$mnemonic" in
    vmovupd)
        stem=latx-avx-single-vmovupd
        expected_bytes=384
        success_cases='xmm-load-u,xmm-store-u,ymm-load-u,ymm-store-u'
        fault_cases='xmm-load-cross,xmm-store-cross,ymm-load-cross,ymm-store-cross,xmm-load-page,xmm-store-page,ymm-load-page,ymm-store-page'
        source_s=latx-avx-single-vmovupd.S
        source_c=latx-avx-single-vmovupd.c
        ;;
    vmovups)
        stem=latx-avx-single-vmovups
        expected_bytes=4544
        success_cases='none'
        fault_cases='xmm-load-cross-1,xmm-load-cross-15,xmm-store-cross-1,xmm-store-cross-15,ymm-load-cross-1,ymm-load-cross-15,ymm-load-cross-16,ymm-load-cross-31,ymm-store-cross-1,ymm-store-cross-15,ymm-store-cross-16,ymm-store-cross-31'
        source_s=latx-avx-single-vmovups.S
        source_c=latx-avx-single-vmovups.c
        ;;
    *) echo "FAIL WI-1897 supports vmovupd/vmovups" >&2; exit 2 ;;
esac
remote_host=$2
remote_dir=${3:-/tmp/latx-avx-wi1897-$mnemonic}
local_output=${4:-/tmp/latx-avx-wi1897-$mnemonic.static}
script_dir=$(cd "$(dirname "$0")" && pwd)
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
for file in "$script_dir/latx-avx-single-runtime.S" "$script_dir/$source_s" "$script_dir/$source_c" "$script_dir/latx-avx-single-common.h" "$script_dir/check-latx-avx-wi1897-mnemonic.sh"; do
    [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
done
ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$script_dir/latx-avx-single-runtime.S" "$script_dir/$source_s" "$script_dir/$source_c" "$script_dir/latx-avx-single-common.h" "$script_dir/check-latx-avx-wi1897-mnemonic.sh" "$remote_host:$remote_dir/"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$stem" "$mnemonic" "$expected_bytes" "$success_cases" "$fault_cases" <<'REMOTE'
set -euo pipefail
remote_dir=$1
stem=$2
mnemonic=$3
expected_bytes=$4
success_cases=$5
fault_cases=$6
docker run --rm --platform linux/amd64 -e STEM="$stem" -e MNEMONIC="$mnemonic" -e EXPECTED_BYTES="$expected_bytes" -e SUCCESS_CASES="$success_cases" -e FAULT_CASES="$fault_cases" -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
    SUCCESS_CASES=${SUCCESS_CASES//,/ }
    FAULT_CASES=${FAULT_CASES//,/ }
    gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone -mgeneral-regs-only -mno-avx -mno-avx2 -I /work -o "/work/$STEM.static" /work/latx-avx-single-runtime.S "/work/$STEM.S" "/work/$STEM.c"
    chmod +x /work/check-latx-avx-wi1897-mnemonic.sh
    /work/check-latx-avx-wi1897-mnemonic.sh "/work/$STEM.static" "$MNEMONIC"
    "/work/$STEM.static" > "/work/$STEM.native"
    test "$(wc -c < "/work/$STEM.native")" -eq "$EXPECTED_BYTES"
    for case_name in $SUCCESS_CASES; do
        test "$case_name" = none && continue
        "/work/$STEM.static" "$case_name"
        test "$?" -eq 0
    done
    for case_name in $FAULT_CASES; do
        if "/work/$STEM.static" "$case_name"; then
            echo "FAIL $case_name must fault" >&2
            exit 1
        else
            test "$?" -eq 139
        fi
    done
    '
REMOTE
mkdir -p "$(dirname "$local_output")"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output.native"
printf 'PASS xzy86 WI-1897 fixture: mnemonic=%s output=%s\n' "$mnemonic" "$local_output"
