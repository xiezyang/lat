#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
output_dir=${1:-/tmp/wi1894-x86-fixtures}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1894-fixtures}
script_dir=$root/tests/integration

bash "$script_dir/test-latx-avx-wi1894-source.sh"
mkdir -p "$output_dir"

for mnemonic in vmaskmovpd vmaskmovps; do
    bash "$script_dir/build-latx-avx-wi1894-xzy86.sh" \
        "$mnemonic" "$remote_host" "$remote_dir/$mnemonic" \
        "$output_dir/latx-avx-single-$mnemonic.static"
done

manifest=$output_dir/native-manifest.tsv
{
    printf 'instruction\tnative_stdout_bytes\tnative_binary_sha256\tnative_stdout_sha256\n'
    for mnemonic in vmaskmovpd vmaskmovps; do
        binary="$output_dir/latx-avx-single-$mnemonic.static"
        native="$binary.native"
        bytes=$(wc -c < "$native")
        [[ "$bytes" -eq 768 ]] || {
            echo "FAIL WI-1894 fixture size: $mnemonic bytes=$bytes" >&2
            exit 1
        }
        printf '%s\t%s\t%s\t%s\n' \
            "$mnemonic" "$bytes" \
            "$(sha256sum "$binary" | awk '{print $1}')" \
            "$(sha256sum "$native" | awk '{print $1}')"
    done
} > "$manifest"

[[ $(awk 'NR > 1 {count++} END {print count + 0}' "$manifest") -eq 2 ]]
printf 'PASS WI-1894 xzy86 fixture set: count=2 manifest=%s\n' "$manifest"
