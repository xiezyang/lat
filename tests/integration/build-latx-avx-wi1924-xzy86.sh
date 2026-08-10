#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT_DIR]" >&2
    exit 2
fi
remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1924-x86}
local_output=${3:-$(mktemp -d /tmp/wi1924-x86-XXXXXX)}
case "$remote_dir" in /tmp/*) ;; *) echo "remote dir must be under /tmp" >&2; exit 2;; esac
case "$local_output" in /tmp/*) ;; *) echo "local output must be under /tmp" >&2; exit 2;; esac

script_dir=$(cd "$(dirname "$0")" && pwd)
fixture_dir=${WI1924_FIXTURE_DIR:-$script_dir/wi1924-fixtures}
for f in latx-avx-single-vzeroall.S latx-avx-single-vzeroall.c; do
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
    "$script_dir/check-latx-avx-wi1924-mnemonic.sh" \
    "$script_dir/check-latx-avx-wi1924-output.sh" \
    "$fixture_dir"/latx-avx-single-vzeroall.* "$remote_host:$remote_dir/"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" <<'REMOTE'
set -euo pipefail
remote_dir=$1
docker run --rm --platform linux/amd64 -v "$remote_dir:/work" -w /work \
    latx-ci-baseline:latest bash -ceu '
set -euo pipefail
stem=latx-avx-single-vzeroall
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
chmod +x /work/check-latx-avx-wi1924-mnemonic.sh /work/check-latx-avx-wi1924-output.sh
set +e
/work/check-latx-avx-wi1924-mnemonic.sh "$binary" vzeroall > /work/$stem.static.stdout 2> /work/$stem.static.stderr
static_rc=$?
set -e
printf "%s\n" "$static_rc" > /work/$stem.static.exit
test "$static_rc" -eq 0
objdump -d -Mintel "$binary" > /work/$stem.objdump
printf "%s\n" "fault=not-applicable" "reason=VZEROALL has no memory operand; no fault case was run" > /work/$stem.fault-not-applicable.txt
printf "%s\n" "$binary" > /work/$stem.normal.command
set +e
"$binary" > /work/$stem.normal.stdout 2> /work/$stem.normal.stderr
normal_rc=$?
set -e
printf "%s\n" "$normal_rc" > /work/$stem.normal.exit
test "$normal_rc" -eq 0
test "$(wc -c < /work/$stem.normal.stdout)" -eq 2112
printf "%s\n" "/work/check-latx-avx-wi1924-output.sh /work/$stem.normal.stdout" > /work/$stem.output-check.command
set +e
/work/check-latx-avx-wi1924-output.sh /work/$stem.normal.stdout > /work/$stem.output-check.stdout 2> /work/$stem.output-check.stderr
output_check_rc=$?
set -e
printf "%s\n" "$output_check_rc" > /work/$stem.output-check.exit
test "$output_check_rc" -eq 0
printf "mnemonic\tartifact\tstatus\tstdout_sha256\tstderr_sha256\n" > /work/manifest.tsv
for artifact in build static normal output-check; do
    rc_file=/work/$stem.$artifact.exit
    rc=$(cat "$rc_file")
    printf "vzeroall\t%s\t%s\t%s\t%s\n" "$artifact" "$rc" \
        "$(sha256sum /work/$stem.$artifact.stdout | cut -d " " -f1)" \
        "$(sha256sum /work/$stem.$artifact.stderr | cut -d " " -f1)" >> /work/manifest.tsv
done
sha256sum /work/$stem.S /work/$stem.c "$binary" /work/$stem.objdump >> /work/$stem.sha256
printf "source_dir=/work\nremote_host=%s\nfault=not-applicable\n" "$(hostname)" >> /work/manifest.tsv
'
REMOTE

mkdir -p "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/manifest.tsv" "$local_output/"
scp "${scp_args[@]}" "$remote_host:$remote_dir/latx-avx-single-vzeroall."* "$local_output/"
printf 'PASS WI-1924 xzy86 baseline output=%s\n' "$local_output"
