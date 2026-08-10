#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
output_dir=${1:-/tmp/wi1899-x86-fixture}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1899-vmaskmovdqu}
script_dir=$root/tests/integration
bash "$script_dir/test-latx-avx-wi1899-source.sh"
mkdir -p "$output_dir"
bash "$script_dir/build-latx-avx-wi1899-xzy86.sh" "$remote_host" "$remote_dir" "$output_dir/latx-avx-single-vmaskmovdqu.static"
binary=$output_dir/latx-avx-single-vmaskmovdqu.static
native=$binary.native
[[ $(wc -c < "$native") -eq 256 ]]
manifest=$output_dir/native-manifest.tsv
printf 'instruction\tnative_stdout_bytes\tnative_binary_sha256\tnative_stdout_sha256\n' > "$manifest"
printf 'vmaskmovdqu\t%s\t%s\t%s\n' "$(wc -c < "$native")" "$(sha256sum "$binary" | awk '{print $1}')" "$(sha256sum "$native" | awk '{print $1}')" >> "$manifest"
printf 'PASS WI-1899 xzy86 fixture set: manifest=%s\n' "$manifest"
