#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
output_dir=${2:-/tmp/wi1898-x86-fixture}
remote_host=${3:-xzy86}
remote_dir=${4:-/tmp/latx-avx-wi1898-vmovdqu}
script_dir=$root/tests/integration

bash "$script_dir/test-latx-avx-wi1898-source.sh" "$root"
mkdir -p "$output_dir"
bash "$script_dir/build-latx-avx-wi1898-xzy86.sh" \
    "$remote_host" "$remote_dir" \
    "$output_dir/latx-avx-single-vmovdqu.static"

binary=$output_dir/latx-avx-single-vmovdqu.static
native=$binary.native
[[ $(wc -c < "$native") -eq 384 ]]
printf 'instruction\tnative_stdout_bytes\tnative_binary_sha256\tnative_stdout_sha256\n' \
    > "$output_dir/native-manifest.tsv"
printf 'vmovdqu\t%s\t%s\t%s\n' \
    "$(wc -c < "$native")" \
    "$(sha256sum "$binary" | awk '{print $1}')" \
    "$(sha256sum "$native" | awk '{print $1}')" \
    >> "$output_dir/native-manifest.tsv"
printf 'PASS WI-1898 xzy86 fixture set: manifest=%s\n' \
    "$output_dir/native-manifest.tsv"
