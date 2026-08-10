#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 4 ]]; then
    echo "usage: $0 MNEMONIC [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT]" >&2
    exit 2
fi
mnemonic=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1914-$mnemonic}
local_output=${4:-/tmp/latx-avx-wi1914-$mnemonic.static}
script_dir=$(cd "$(dirname "$0")" && pwd)
fixture_dir=${WI1914_FIXTURE_DIR:-$script_dir/wi1914-fixtures}
stem="latx-avx-single-$mnemonic"
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi

for file in \
    "$script_dir/latx-avx-single-runtime.S" \
    "$fixture_dir/$stem.S" "$fixture_dir/$stem.c" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1914-mnemonic.sh"; do
    [[ -f "$file" ]] || { echo "FAIL missing fixture: $file" >&2; exit 2; }
done

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" "$fixture_dir/$stem.S" \
    "$fixture_dir/$stem.c" "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1914-mnemonic.sh" \
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
    chmod +x /work/check-latx-avx-wi1914-mnemonic.sh
    /work/check-latx-avx-wi1914-mnemonic.sh "/work/$STEM.static" "$MNEMONIC"
    "/work/$STEM.static" reference > "/work/$STEM.native"
    test "$(wc -c < "/work/$STEM.native")" -eq 384
    set +e
    "/work/$STEM.static" fault-xmm > "/work/$STEM.fault-xmm.stdout" 2> "/work/$STEM.fault-xmm.stderr"
    rc=$?
    set -e
    test "$rc" -eq 139
    printf "%s\\n" "$rc" > "/work/$STEM.fault-xmm.status"
    if grep -q "fault_ymm" "/work/$STEM.S"; then
        set +e
        "/work/$STEM.static" fault-ymm > "/work/$STEM.fault-ymm.stdout" 2> "/work/$STEM.fault-ymm.stderr"
        rc=$?
        set -e
        test "$rc" -eq 139
        printf "%s\\n" "$rc" > "/work/$STEM.fault-ymm.status"
    fi
    '
REMOTE

mkdir -p "$(dirname "$local_output")"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.static" "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.native" "$local_output.native"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.fault-xmm.status" \
    "$local_output.fault-xmm.status"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.fault-xmm.stdout" \
    "$local_output.fault-xmm.stdout"
scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.fault-xmm.stderr" \
    "$local_output.fault-xmm.stderr"
if ssh "${ssh_args[@]}" "$remote_host" test -f "$remote_dir/$stem.fault-ymm.status"; then
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.fault-ymm.status" \
        "$local_output.fault-ymm.status"
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.fault-ymm.stdout" \
        "$local_output.fault-ymm.stdout"
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$stem.fault-ymm.stderr" \
        "$local_output.fault-ymm.stderr"
fi
sha256sum "$local_output" > "$local_output.sha256"
printf 'PASS WI-1914 xzy86 fixture: mnemonic=%s output=%s\n' "$mnemonic" "$local_output"
