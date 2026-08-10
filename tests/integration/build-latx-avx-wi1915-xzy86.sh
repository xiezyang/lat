#!/usr/bin/env bash
set -euo pipefail
[ "$#" -eq 3 ] || { echo "usage: $0 REMOTE_HOST REMOTE_DIR LOCAL_OUTPUT_DIR" >&2; exit 2; }
remote_host=$1
remote_dir=$2
local_output_dir=$3
case "$remote_dir" in /tmp/*) ;; *) exit 2 ;; esac
case "$local_output_dir" in /tmp/*) ;; *) exit 2 ;; esac
script_dir=$(cd "$(dirname "$0")" && pwd)
mnemonics=$(python3 "$script_dir/generate-latx-avx-wi1915-fixtures.py" | sed -n 's/^mnemonics=//p')
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
files=("$script_dir/latx-avx-single-runtime.S" "$script_dir/latx-avx-single-common.h" "$script_dir/check-latx-avx-wi1915-mnemonic.sh")
for m in $mnemonics; do
    files+=("$script_dir/latx-avx-single-$m.S" "$script_dir/latx-avx-single-$m.c")
done
scp "${scp_args[@]}" "${files[@]}" "$remote_host:$remote_dir/"
mkdir -p "$local_output_dir"
mnemonics_b64=$(printf '%s' "$mnemonics" | base64 -w0)
set +e
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$mnemonics_b64" <<'REMOTE'
set -euo pipefail
remote_dir=$1
mnemonics=$(printf '%s' "$2" | base64 -d)
docker run --rm --platform linux/amd64 -e MNEMONICS="$mnemonics" -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
set -euo pipefail
for m in $MNEMONICS; do
    stem=latx-avx-single-$m
    gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
        -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
        -mgeneral-regs-only -mno-avx -mno-avx2 -I /work \
        -o /work/$stem.static /work/latx-avx-single-runtime.S \
        /work/$stem.S /work/$stem.c > /work/$stem.build.stdout \
        2> /work/$stem.build.stderr
    chmod +x /work/check-latx-avx-wi1915-mnemonic.sh
    /work/check-latx-avx-wi1915-mnemonic.sh /work/$stem.static $m \
        > /work/$stem.static.stdout 2> /work/$stem.static.stderr
    set +e
    /work/$stem.static > /work/$stem.native 2> /work/$stem.native.stderr
    n=$?
    /work/$stem.static fault > /work/$stem.fault.stdout 2> /work/$stem.fault.stderr
    f=$?
    set -e
    printf "%s\n" "$n" > /work/$stem.normal.status
    printf "%s\n" "$f" > /work/$stem.fault.status
    test "$n" -eq 0
    if [ "$m" = vpmovmskb ]; then
        test "$f" -eq 1
    else
        test "$f" -eq 139
    fi
done
'
REMOTE
status=$?
set -e
mkdir -p "$local_output_dir"
printf '%s\n' "$status" > "$local_output_dir/build.status"
printf '%s\n' "$status" > "$local_output_dir/build.exit"
scp "${scp_args[@]}" "$remote_host:$remote_dir/latx-avx-single-*" "$local_output_dir/" \
    >/dev/null 2>> "$local_output_dir/scp.stderr" || true
[ "$status" -eq 0 ] || { echo "FAIL WI-1915 xzy86 build/run status=$status" >&2; exit "$status"; }
printf 'PASS WI-1915 xzy86 build/run output=%s\n' "$local_output_dir"
