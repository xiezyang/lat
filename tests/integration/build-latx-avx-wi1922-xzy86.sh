#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT_DIR]" >&2
    exit 2
fi
remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1922-x86}
local_output=${3:-$(mktemp -d /tmp/wi1922-x86-XXXXXX)}
case "$remote_dir" in /tmp/*) ;; *) echo "remote dir must be under /tmp" >&2; exit 2;; esac
case "$local_output" in /tmp/*) ;; *) echo "local output must be under /tmp" >&2; exit 2;; esac

script_dir=$(cd "$(dirname "$0")" && pwd)
fixture_dir=${WI1922_FIXTURE_DIR:-$script_dir/wi1922-fixtures}
for f in latx-avx-single-vmpsadbw.S latx-avx-single-vmpsadbw.c; do
    [[ -f "$fixture_dir/$f" ]] || exit 2
done

ssh_args=(); scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1922-mnemonic.sh" \
    "$fixture_dir"/latx-avx-single-vmpsadbw.* "$remote_host:$remote_dir/"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" <<'REMOTE'
set -euo pipefail
remote_dir=$1
docker run --rm --platform linux/amd64 -v "$remote_dir:/work" -w /work \
    latx-ci-baseline:latest bash -ceu '
set -euo pipefail
stem=latx-avx-single-vmpsadbw
binary=/work/$stem.static
printf "%s\n" "gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone -mgeneral-regs-only -mno-avx -mno-avx2 -I /work /work/latx-avx-single-runtime.S /work/$stem.S /work/$stem.c -o $binary" > /work/$stem.build.command
set +e
gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
    -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
    -mgeneral-regs-only -mno-avx -mno-avx2 -I /work \
    /work/latx-avx-single-runtime.S /work/$stem.S /work/$stem.c \
    -o "$binary" > /work/$stem.build.stdout 2> /work/$stem.build.stderr
build_rc=$?
set -e
printf "%s\n" "$build_rc" > /work/$stem.build.exit
test "$build_rc" -eq 0
chmod +x /work/check-latx-avx-wi1922-mnemonic.sh
set +e
/work/check-latx-avx-wi1922-mnemonic.sh "$binary" vmpsadbw > /work/$stem.static.stdout 2> /work/$stem.static.stderr
static_rc=$?
set -e
printf "%s\n" "$static_rc" > /work/$stem.static.exit
test "$static_rc" -eq 0
objdump -d -Mintel "$binary" > /work/$stem.objdump
printf "mnemonic\tartifact\tstatus\tstdout_sha256\tstderr_sha256\n" > /work/manifest.tsv
for mode in normal xmm-cross-8 ymm-cross-16; do
    printf "%s\n" "$binary ${mode/normal/}" > /work/$stem.$mode.command
    set +e
    if [ "$mode" = normal ]; then "$binary" > /work/$stem.$mode.stdout 2> /work/$stem.$mode.stderr; else "$binary" "$mode" > /work/$stem.$mode.stdout 2> /work/$stem.$mode.stderr; fi
    rc=$?
    set -e
    printf "%s\n" "$rc" > /work/$stem.$mode.exit
    printf "vmpsadbw\t%s\t%s\t%s\t%s\n" "$mode" "$rc" \
        "$(sha256sum /work/$stem.$mode.stdout | cut -d " " -f1)" \
        "$(sha256sum /work/$stem.$mode.stderr | cut -d " " -f1)" >> /work/manifest.tsv
    case "$mode" in
        normal) test "$rc" -eq 0; test "$(wc -c < /work/$stem.$mode.stdout)" -eq 640 ;;
        *) test "$rc" -eq 139; test "$(wc -c < /work/$stem.$mode.stdout)" -eq 16 ;;
    esac
done
sha256sum /work/$stem.S /work/$stem.c "$binary" /work/$stem.objdump > /work/$stem.sha256
printf "source_dir=/work\nremote_host=%s\n" "$(hostname)" >> /work/manifest.tsv
'
REMOTE

mkdir -p "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/manifest.tsv" "$local_output/"
scp "${scp_args[@]}" "$remote_host:$remote_dir/latx-avx-single-vmpsadbw."* "$local_output/"
printf 'PASS WI-1922 xzy86 baseline output=%s\n' "$local_output"
