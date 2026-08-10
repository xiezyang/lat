#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
output_dir=${1:-/tmp/wi1904-x86-fixtures}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1904}
script_dir=$root/tests/integration

bash "$script_dir/test-latx-avx-wi1904-source.sh"
bash -n \
    "$script_dir/check-latx-avx-wi1904-mnemonic.sh" \
    "$script_dir/build-latx-avx-wi1904-xzy86.sh" \
    "$script_dir/test-latx-avx-wi1904-source.sh" \
    "$script_dir/test-latx-avx-wi1904-fixtures.sh"
bash "$script_dir/build-latx-avx-wi1904-xzy86.sh" \
    "$remote_host" "$remote_dir" "$output_dir"

printf 'instruction\tnative_stdout_bytes\tbinary_sha256\tnative_sha256\n' \
    > "$output_dir/native-manifest.tsv"
for mnemonic in vmovhpd vmovlpd; do
    binary="$output_dir/latx-avx-single-$mnemonic.static"
    native="$binary.native"
    printf '%s\t%s\t%s\t%s\n' \
        "$mnemonic" "$(wc -c < "$native")" \
        "$(sha256sum "$binary" | awk '{print $1}')" \
        "$(sha256sum "$native" | awk '{print $1}')" \
        >> "$output_dir/native-manifest.tsv"
done
[[ $(awk 'NR > 1 {count++} END {print count + 0}' \
    "$output_dir/native-manifest.tsv") -eq 2 ]]
printf 'PASS WI-1904 xzy86 fixture set: count=2 manifest=%s\n' \
    "$output_dir/native-manifest.tsv"
