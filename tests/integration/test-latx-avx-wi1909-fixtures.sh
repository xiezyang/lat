#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
output_dir=${1:-/tmp/wi1909-x86-fixtures}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1909}
script_dir=$root/tests/integration
bash "$script_dir/test-latx-avx-wi1909-source.sh"
bash -n "$script_dir/check-latx-avx-wi1909-mnemonic.sh" \
    "$script_dir/build-latx-avx-wi1909-xzy86.sh" \
    "$script_dir/test-latx-avx-wi1909-source.sh" \
    "$script_dir/test-latx-avx-wi1909-fixtures.sh"
python3 -m json.tool "$script_dir/latx-avx-opt-only-manifest.json" >/dev/null
rm -rf "$output_dir"
bash "$script_dir/build-latx-avx-wi1909-xzy86.sh" "$remote_host" \
    "$remote_dir" "$output_dir"
manifest=$output_dir/native-manifest.tsv
printf 'instruction\tnative_stdout_bytes\tnative_binary_sha256\tnative_stdout_sha256\n' > "$manifest"
for mnemonic in vmovshdup vmovsldup; do
    binary="$output_dir/latx-avx-single-$mnemonic.static"
    native="$binary.native"
    printf '%s\t%s\t%s\t%s\n' "$mnemonic" "$(wc -c < "$native")" \
        "$(sha256sum "$binary" | awk '{print $1}')" \
        "$(sha256sum "$native" | awk '{print $1}')" >> "$manifest"
done
[[ $(awk 'NR > 1 {count++} END {print count + 0}' "$manifest") -eq 2 ]]
printf 'PASS WI-1909 xzy86 fixture set: count=2 manifest=%s\n' "$manifest"
