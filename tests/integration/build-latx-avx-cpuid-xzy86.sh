#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 4 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT_DIR] [IMAGE]" >&2
    exit 2
fi

remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-cpuid-wi1847}
local_output=${3:-/tmp/latx-avx-cpuid-wi1847}
image=${4:-latx-ci-baseline:latest}
script_dir=$(cd "$(dirname "$0")" && pwd)
repo_dir=$(cd "$script_dir/../.." && pwd)
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi

for file in \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-cpuid-probe.S" \
    "$script_dir/latx-avx-cpuid-probe.c"; do
    [[ -f "$file" ]] || {
        echo "FAIL missing probe source: $file" >&2
        exit 2
    }
done

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-cpuid-probe.S" \
    "$script_dir/latx-avx-cpuid-probe.c" \
    "$remote_host:$remote_dir/"

ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$image" <<'REMOTE'
set -euo pipefail
remote_dir=$1
image=$2

docker run --rm -e IMAGE="$image" -v "$remote_dir:/work" -w /work "$image" \
    bash -ceu '
        gcc -std=c11 -O0 -Wall -Wextra -Werror \
            -static -no-pie -ffreestanding -fno-builtin \
            -fno-stack-protector -mno-red-zone -mgeneral-regs-only \
            -mno-avx -mno-avx2 -o /work/latx-avx-cpuid-probe.static \
            /work/latx-avx-single-runtime.S \
            /work/latx-avx-cpuid-probe.S \
            /work/latx-avx-cpuid-probe.c
        chmod +x /work/latx-avx-cpuid-probe.static
        sha256sum /work/latx-avx-cpuid-probe.static \
            /work/latx-avx-single-runtime.S \
            /work/latx-avx-cpuid-probe.S \
            /work/latx-avx-cpuid-probe.c > /work/source-and-probe.sha256
        /work/latx-avx-cpuid-probe.static info > /work/x86-info.stdout 2> /work/x86-info.stderr
        /work/latx-avx-cpuid-probe.static xgetbv > /work/x86-xgetbv.stdout 2> /work/x86-xgetbv.stderr
        printf "%s\n" "$?" > /work/x86-xgetbv.status
        set +e
        /work/latx-avx-cpuid-probe.static guarded > /work/x86-guarded.stdout 2> /work/x86-guarded.stderr
        printf "%s\n" "$?" > /work/x86-guarded.status
        /work/latx-avx-cpuid-probe.static unconditional > /work/x86-unconditional.stdout 2> /work/x86-unconditional.stderr
        printf "%s\n" "$?" > /work/x86-unconditional.status
        /work/latx-avx-cpuid-probe.static bmi2 > /work/x86-bmi2.stdout 2> /work/x86-bmi2.stderr
        printf "%s\n" "$?" > /work/x86-bmi2.status
        set -e
    '
REMOTE

mkdir -p "$local_output"
scp "${scp_args[@]}" "$remote_host:$remote_dir/latx-avx-cpuid-probe.static" \
    "$local_output/latx-avx-cpuid-probe.static"
for file in source-and-probe.sha256 \
    x86-info.stdout x86-info.stderr x86-xgetbv.stdout x86-xgetbv.stderr \
    x86-guarded.stdout x86-guarded.stderr x86-guarded.status \
    x86-unconditional.stdout x86-unconditional.stderr x86-unconditional.status \
    x86-bmi2.stdout x86-bmi2.stderr x86-bmi2.status; do
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$file" "$local_output/$file"
done

printf 'PASS xzy86 native fixture prepared: %s\n' "$local_output/latx-avx-cpuid-probe.static"
printf 'source=%s/source-and-probe.sha256\n' "$local_output"
