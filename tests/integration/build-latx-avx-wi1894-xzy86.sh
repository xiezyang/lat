#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 4 ]]; then
    echo "usage: $0 MNEMONIC [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT]" >&2
    exit 2
fi

mnemonic=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')
case "$mnemonic" in
    vmaskmovpd|vmaskmovps) ;;
    *) echo "FAIL WI-1894 only supports vmaskmovpd/vmaskmovps" >&2; exit 2 ;;
esac
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1894-$mnemonic}
local_output=${4:-/tmp/latx-avx-wi1894-$mnemonic.static}
script_dir=$(cd "$(dirname "$0")" && pwd)
stem="latx-avx-single-$mnemonic"
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
    "$script_dir/check-latx-avx-wi1894-mnemonic.sh"; do
    [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
done

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/$stem.S" \
    "$script_dir/$stem.c" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1894-mnemonic.sh" \
    "$remote_host:$remote_dir/"

ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$stem" "$mnemonic" <<'REMOTE'
set -euo pipefail
remote_dir=$1
stem=$2
mnemonic=$3
docker run --rm --platform linux/amd64 \
    -e STEM="$stem" -e MNEMONIC="$mnemonic" \
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
        chmod +x /work/check-latx-avx-wi1894-mnemonic.sh
        /work/check-latx-avx-wi1894-mnemonic.sh \
            "/work/$STEM.static" "$MNEMONIC"
        "/work/$STEM.static" > "/work/$STEM.native"
        test "$(wc -c < "/work/$STEM.native")" -eq 768
        "/work/$STEM.static" a
        "/work/$STEM.static" b
        if "/work/$STEM.static" c; then
            echo "FAIL masked-on load must fault" >&2
            exit 1
        else
            test "$?" -eq 139
        fi
        if "/work/$STEM.static" d; then
            echo "FAIL masked-on store must fault" >&2
            exit 1
        else
            test "$?" -eq 139
        fi
    '
REMOTE

mkdir -p "$(dirname "$local_output")"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output.native"
printf 'PASS xzy86 WI-1894 fixture: mnemonic=%s output=%s\n' \
    "$mnemonic" "$local_output"
