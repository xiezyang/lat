#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT_DIR]" >&2
    exit 2
fi
remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1920-x86}
local_output=${3:-$(mktemp -d /tmp/wi1920-x86-XXXXXX)}
case "$remote_dir" in /tmp/*) ;; *) echo "remote dir must be under /tmp" >&2; exit 2;; esac
case "$local_output" in /tmp/*) ;; *) echo "local output must be under /tmp" >&2; exit 2;; esac

script_dir=$(cd "$(dirname "$0")" && pwd)
fixture_dir=${WI1920_FIXTURE_DIR:-$script_dir/wi1920-fixtures}
mnemonics=(vunpckhpd vunpckhps vunpcklpd vunpcklps)
ssh_args=(); scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
for m in "${mnemonics[@]}"; do
    [[ -f "$fixture_dir/latx-avx-single-$m.S" ]] || exit 2
    [[ -f "$fixture_dir/latx-avx-single-$m.c" ]] || exit 2
done

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/check-latx-avx-wi1920-mnemonic.sh" \
    "$fixture_dir"/latx-avx-single-* "$remote_host:$remote_dir/"
mnemonic_list=$(IFS=,; printf '%s' "${mnemonics[*]}")
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$mnemonic_list" <<'REMOTE'
set -euo pipefail
remote_dir=$1
mnemonics=$(printf '%s' "$2" | tr ',' ' ')
docker run --rm --platform linux/amd64 -e MNEMONICS="$mnemonics" \
    -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
set -euo pipefail
printf "mnemonic\tartifact\tstatus\tstdout_sha256\tstderr_sha256\n" > /work/manifest.tsv
for m in $MNEMONICS; do
    stem=latx-avx-single-$m
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
    printf "%s\tbuild\t%s\t%s\t%s\n" "$m" "$build_rc" \
        "$(sha256sum /work/$stem.build.stdout | cut -d " " -f1)" \
        "$(sha256sum /work/$stem.build.stderr | cut -d " " -f1)" >> /work/manifest.tsv
    test "$build_rc" -eq 0
    chmod +x /work/check-latx-avx-wi1920-mnemonic.sh
    set +e
    /work/check-latx-avx-wi1920-mnemonic.sh "$binary" "$m" > /work/$stem.static.stdout 2> /work/$stem.static.stderr
    static_rc=$?
    set -e
    printf "%s\n" "$static_rc" > /work/$stem.static.exit
    test "$static_rc" -eq 0
    objdump -d -Mintel "$binary" > /work/$stem.objdump
    for mode in normal xmm-cross-8 ymm-cross-16; do
        printf "%s\n" "$binary ${mode/normal/}" > /work/$stem.$mode.command
        set +e
        if [ "$mode" = normal ]; then "$binary" > /work/$stem.$mode.stdout 2> /work/$stem.$mode.stderr; else "$binary" "$mode" > /work/$stem.$mode.stdout 2> /work/$stem.$mode.stderr; fi
        rc=$?
        set -e
        printf "%s\n" "$rc" > /work/$stem.$mode.exit
        printf "%s\t%s\t%s\t%s\t%s\n" "$m" "$mode" "$rc" \
            "$(sha256sum /work/$stem.$mode.stdout | cut -d " " -f1)" \
            "$(sha256sum /work/$stem.$mode.stderr | cut -d " " -f1)" >> /work/manifest.tsv
        case "$mode" in
            normal) test "$rc" -eq 0; test "$(wc -c < /work/$stem.$mode.stdout)" -eq 384 ;;
            *) test "$rc" -eq 139; test "$(wc -c < /work/$stem.$mode.stdout)" -eq 16 ;;
        esac
    done
    sha256sum /work/$stem.S /work/$stem.c "$binary" /work/$stem.objdump >> /work/$stem.sha256
done
printf "source_dir=/work\nremote_host=%s\n" "$(hostname)" >> /work/manifest.tsv
'
REMOTE

mkdir -p "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/manifest.tsv" "$local_output/"
for m in "${mnemonics[@]}"; do
    scp "${scp_args[@]}" "$remote_host:$remote_dir/latx-avx-single-$m."* "$local_output/"
done
printf 'PASS WI-1920 xzy86 baseline output=%s\n' "$local_output"
