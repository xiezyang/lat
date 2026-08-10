#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 4 ]]; then
    echo "usage: $0 REMOTE_HOST REMOTE_DIR LOCAL_OUTPUT_DIR [IMAGE]" >&2
    exit 2
fi

remote_host=$1
remote_dir=$2
local_output=$3
image=${4:-latx-ci-baseline:latest}
script_dir=$(cd "$(dirname "$0")" && pwd)
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi

for file in \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/latx-avx-single-vcvtsd2ss.S" \
    "$script_dir/latx-avx-single-vcvtsd2ss.c" \
    "$script_dir/latx-avx-single-vcvtsd2ss-daz.S" \
    "$script_dir/latx-avx-single-vcvtsd2ss-daz.c"; do
    [[ -f "$file" ]] || { echo "FAIL missing source: $file" >&2; exit 2; }
done

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/latx-avx-single-vcvtsd2ss.S" \
    "$script_dir/latx-avx-single-vcvtsd2ss.c" \
    "$script_dir/latx-avx-single-vcvtsd2ss-daz.S" \
    "$script_dir/latx-avx-single-vcvtsd2ss-daz.c" \
    "$remote_host:$remote_dir/"

ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$image" <<'REMOTE'
set -euo pipefail
remote_dir=$1
image=$2

docker run --rm -v "$remote_dir:/work" -w /work "$image" bash -ceu '
    common="-std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
        -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
        -mgeneral-regs-only -mno-avx -mno-avx2 -I /work"
    gcc $common -o /work/latx-avx-single-vcvtsd2ss.static \
        /work/latx-avx-single-runtime.S \
        /work/latx-avx-single-vcvtsd2ss.S \
        /work/latx-avx-single-vcvtsd2ss.c
    gcc $common -o /work/latx-avx-single-vcvtsd2ss-daz.static \
        /work/latx-avx-single-runtime.S \
        /work/latx-avx-single-vcvtsd2ss-daz.S \
        /work/latx-avx-single-vcvtsd2ss-daz.c
    chmod +x /work/latx-avx-single-vcvtsd2ss*.static
    mkdir -p /work/x86-vcvtsd2ss /work/x86-vcvtsd2ss-daz
    sha256sum /work/latx-avx-single-runtime.S \
        /work/latx-avx-single-common.h \
        /work/latx-avx-single-vcvtsd2ss.S \
        /work/latx-avx-single-vcvtsd2ss.c \
        /work/latx-avx-single-vcvtsd2ss-daz.S \
        /work/latx-avx-single-vcvtsd2ss-daz.c \
        /work/latx-avx-single-vcvtsd2ss.static \
        /work/latx-avx-single-vcvtsd2ss-daz.static \
        > /work/source-and-probe.sha256

    run_case() {
        dir=$1
        probe=$2
        name=$3
        set +e
        "/work/$probe" "$name" > "/work/$dir/$name.bin" \
            2> "/work/$dir/$name.stderr"
        rc=$?
        set -e
        printf "%s\n" "$rc" > "/work/$dir/$name.status"
    }
    run_case x86-vcvtsd2ss latx-avx-single-vcvtsd2ss.static reference
    run_case x86-vcvtsd2ss latx-avx-single-vcvtsd2ss.static fpe-invalid
    run_case x86-vcvtsd2ss latx-avx-single-vcvtsd2ss.static fpe-precision
    run_case x86-vcvtsd2ss latx-avx-single-vcvtsd2ss.static fpe-overflow
    run_case x86-vcvtsd2ss latx-avx-single-vcvtsd2ss.static fpe-underflow
    run_case x86-vcvtsd2ss-daz latx-avx-single-vcvtsd2ss-daz.static reference
    {
        printf "host=xzy86\n"
        printf "compile= gcc %s ...\n" "$common"
        printf "reference and fault cases were run natively on x86\n"
    } > /work/x86-commands.txt
' 
REMOTE

mkdir -p "$local_output/vcvtsd2ss" "$local_output/vcvtsd2ss-daz"
for file in \
    source-and-probe.sha256 x86-commands.txt \
    x86-vcvtsd2ss/reference.bin x86-vcvtsd2ss/reference.stderr \
    x86-vcvtsd2ss/reference.status x86-vcvtsd2ss/fpe-invalid.bin \
    x86-vcvtsd2ss/fpe-invalid.stderr x86-vcvtsd2ss/fpe-invalid.status \
    x86-vcvtsd2ss/fpe-precision.bin x86-vcvtsd2ss/fpe-precision.stderr \
    x86-vcvtsd2ss/fpe-precision.status x86-vcvtsd2ss/fpe-overflow.bin \
    x86-vcvtsd2ss/fpe-overflow.stderr x86-vcvtsd2ss/fpe-overflow.status \
    x86-vcvtsd2ss/fpe-underflow.bin x86-vcvtsd2ss/fpe-underflow.stderr \
    x86-vcvtsd2ss/fpe-underflow.status \
    x86-vcvtsd2ss-daz/reference.bin x86-vcvtsd2ss-daz/reference.stderr \
    x86-vcvtsd2ss-daz/reference.status; do
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$file" "$local_output/$file"
done

printf 'PASS xzy86 native VCVTSD2SS fixture prepared: %s\n' "$local_output"
