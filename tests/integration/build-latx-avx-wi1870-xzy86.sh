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
    "$script_dir/latx-avx-wi1870-comis.S" \
    "$script_dir/latx-avx-wi1870-comis.c"; do
    [[ -f "$file" ]] || { echo "FAIL missing source: $file" >&2; exit 2; }
done

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" \
    "$script_dir/latx-avx-single-runtime.S" \
    "$script_dir/latx-avx-single-common.h" \
    "$script_dir/latx-avx-wi1870-comis.S" \
    "$script_dir/latx-avx-wi1870-comis.c" \
    "$script_dir/wi1870-comis-fixtures.json" \
    "$remote_host:$remote_dir/"

ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" "$image" <<'REMOTE'
set -euo pipefail
remote_dir=$1
image=$2

docker run --rm -v "$remote_dir:/work" -w /work "$image" bash -ceu '
    common="-std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
        -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
        -mgeneral-regs-only -mno-avx -mno-avx2 -I /work"
    for item in vcomisd vcomiss vucomiss; do
        case "$item" in
            vcomisd) define=WI1870_VCOMISD ;;
            vcomiss) define=WI1870_VCOMISS ;;
            vucomiss) define=WI1870_VUCOMISS ;;
        esac
        mkdir -p "/work/$item"
        gcc $common -D"$define" -o "/work/$item/latx-avx-$item.static" \
            /work/latx-avx-single-runtime.S \
            /work/latx-avx-wi1870-comis.S \
            /work/latx-avx-wi1870-comis.c
        chmod +x "/work/$item/latx-avx-$item.static"
        set +e
        "/work/$item/latx-avx-$item.static" reference \
            > "/work/$item/reference.bin" 2> "/work/$item/reference.stderr"
        printf "%s\n" "$?" > "/work/$item/reference.status"
        "/work/$item/latx-avx-$item.static" fpe-invalid \
            > "/work/$item/fpe-invalid.bin" 2> "/work/$item/fpe-invalid.stderr"
        printf "%s\n" "$?" > "/work/$item/fpe-invalid.status"
        "/work/$item/latx-avx-$item.static" fpe-denormal \
            > "/work/$item/fpe-denormal.bin" 2> "/work/$item/fpe-denormal.stderr"
        printf "%s\n" "$?" > "/work/$item/fpe-denormal.status"
        set -e
    done
    sha256sum /work/latx-avx-single-runtime.S \
        /work/latx-avx-single-common.h /work/latx-avx-wi1870-comis.S \
        /work/latx-avx-wi1870-comis.c /work/wi1870-comis-fixtures.json \
        /work/vcomisd/latx-avx-vcomisd.static \
        /work/vcomiss/latx-avx-vcomiss.static \
        /work/vucomiss/latx-avx-vucomiss.static \
        > /work/source-and-probe.sha256
    printf "host=xzy86\n" > /work/x86-commands.txt
' 
REMOTE

mkdir -p "$local_output"
for file in source-and-probe.sha256 x86-commands.txt; do
    scp "${scp_args[@]}" "$remote_host:$remote_dir/$file" "$local_output/$file"
done
for item in vcomisd vcomiss vucomiss; do
    mkdir -p "$local_output/$item"
    for file in \
        "latx-avx-$item.static" reference.bin reference.stderr reference.status \
        fpe-invalid.bin fpe-invalid.stderr fpe-invalid.status \
        fpe-denormal.bin fpe-denormal.stderr fpe-denormal.status; do
        scp "${scp_args[@]}" "$remote_host:$remote_dir/$item/$file" \
            "$local_output/$item/$file"
    done
done

printf 'PASS xzy86 native WI-1870 fixtures prepared: %s\n' "$local_output"
