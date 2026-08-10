#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
output_dir=${1:-/tmp/wi1895-x86-fixture}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1895-vmovapd}
script_dir=$root/tests/integration
bash "$script_dir/test-latx-avx-wi1895-source.sh"
mkdir -p "$output_dir"
bash "$script_dir/build-latx-avx-wi1895-xzy86.sh" "$remote_host" "$remote_dir" "$output_dir/latx-avx-single-vmovapd.static"

binary=$output_dir/latx-avx-single-vmovapd.static
native=$binary.native
[[ $(wc -c < "$native") -eq 384 ]]
printf 'instruction\tnative_stdout_bytes\tnative_binary_sha256\tnative_stdout_sha256\n' > "$output_dir/native-manifest.tsv"
printf 'vmovapd\t%s\t%s\t%s\n' "$(wc -c < "$native")" "$(sha256sum "$binary" | awk '{print $1}')" "$(sha256sum "$native" | awk '{print $1}')" >> "$output_dir/native-manifest.tsv"
printf 'PASS WI-1895 xzy86 fixture set: manifest=%s\n' "$output_dir/native-manifest.tsv"
