#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
output_dir=${1:-/tmp/wi1897-x86-fixtures}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1897-fixtures}
script_dir=$root/tests/integration
bash "$script_dir/test-latx-avx-wi1897-source.sh"
mkdir -p "$output_dir"
for mnemonic in vmovupd vmovups; do
    bash "$script_dir/build-latx-avx-wi1897-xzy86.sh" "$mnemonic" "$remote_host" "$remote_dir/$mnemonic" "$output_dir/latx-avx-single-$mnemonic.static"
done
manifest=$output_dir/native-manifest.tsv
printf 'instruction\tnative_stdout_bytes\tnative_binary_sha256\tnative_stdout_sha256\n' > "$manifest"
for mnemonic in vmovupd vmovups; do
    binary="$output_dir/latx-avx-single-$mnemonic.static"
    native="$binary.native"
    printf '%s\t%s\t%s\t%s\n' "$mnemonic" "$(wc -c < "$native")" "$(sha256sum "$binary" | awk '{print $1}')" "$(sha256sum "$native" | awk '{print $1}')" >> "$manifest"
done
[[ $(awk 'NR > 1 {count++} END {print count + 0}' "$manifest") -eq 2 ]]
printf 'PASS WI-1897 xzy86 fixture set: count=2 manifest=%s\n' "$manifest"
