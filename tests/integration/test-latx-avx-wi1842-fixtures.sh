#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
output_dir=${1:-/tmp/wi1842-x86-fixtures}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1842-fixtures}
script_dir=$root/tests/integration

mnemonics='vpslld vpslldq vpsllq vpsllvd vpsllvq vpsllw vpsrad vpsravd vpsraw vpsrld vpsrldq vpsrlq vpsrlvd vpsrlvq vpsrlw'

python3 "$script_dir/generate-latx-avx-wi1842-fixtures.py" \
    --output-dir "$script_dir"
bash "$script_dir/test-latx-avx-wi1842-source.sh"
mkdir -p "$output_dir"

for mnemonic in $mnemonics; do
    bash "$script_dir/build-latx-avx-wi1842-xzy86.sh" \
        "$mnemonic" "$remote_host" "$remote_dir/$mnemonic" \
        "$output_dir/latx-avx-single-$mnemonic.static"
done

manifest=$output_dir/native-manifest.tsv
{
    printf 'instruction\tnative_stdout_bytes\tnative_binary_sha256\tnative_stdout_sha256\n'
    for mnemonic in $mnemonics; do
        binary="$output_dir/latx-avx-single-$mnemonic.static"
        native="$binary.native"
        bytes=$(wc -c < "$native")
        binary_sha=$(sha256sum "$binary" | awk '{print $1}')
        native_sha=$(sha256sum "$native" | awk '{print $1}')
        case "$mnemonic" in
            vpslld|vpsllq|vpsllw|vpsrad|vpsraw|vpsrld|vpsrlq|vpsrlw)
                expected=512 ;;
            *)
                expected=256 ;;
        esac
        [[ "$bytes" -eq "$expected" ]] || {
            echo "FAIL WI-1842 fixture size: $mnemonic bytes=$bytes expected=$expected" >&2
            exit 1
        }
        printf '%s\t%s\t%s\t%s\n' \
            "$mnemonic" "$bytes" "$binary_sha" "$native_sha"
    done
} > "$manifest"

[[ $(awk 'NR > 1 {count++} END {print count + 0}' "$manifest") -eq 15 ]]
printf 'PASS WI-1842 xzy86 fixture set: count=15 manifest=%s\n' "$manifest"
