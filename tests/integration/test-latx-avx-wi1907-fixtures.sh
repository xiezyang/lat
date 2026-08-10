#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
output_dir=${1:-/tmp/wi1907-x86-fixture}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1907-vmovntdqa}
script_dir=$root/tests/integration
bash "$script_dir/test-latx-avx-wi1907-source.sh"
bash -n "$script_dir/check-latx-avx-wi1907-mnemonic.sh" \
    "$script_dir/build-latx-avx-wi1907-xzy86.sh" \
    "$script_dir/test-latx-avx-wi1907-source.sh" \
    "$script_dir/test-latx-avx-wi1907-fixtures.sh"
python3 -m json.tool "$script_dir/latx-avx-opt-only-manifest.json" >/dev/null
mkdir -p "$output_dir"
bash "$script_dir/build-latx-avx-wi1907-xzy86.sh" "$remote_host" \
    "$remote_dir" "$output_dir/latx-avx-single-vmovntdqa.static"
binary=$output_dir/latx-avx-single-vmovntdqa.static
native=$binary.native
python3 - "$native" <<'PY'
import sys
data = open(sys.argv[1], "rb").read()
assert len(data) == 128
assert data[:32] == data[64:96]
assert data[32:64] == data[96:128]
print("PASS WI-1907 VMOVNTDQA hint-result equivalence: XMM/YMM records match VMOVDQU")
PY
manifest=$output_dir/native-manifest.tsv
printf 'instruction\tnative_stdout_bytes\tnative_binary_sha256\tnative_stdout_sha256\n' > "$manifest"
printf 'vmovntdqa\t%s\t%s\t%s\n' "$(wc -c < "$native")" \
    "$(sha256sum "$binary" | awk '{print $1}')" \
    "$(sha256sum "$native" | awk '{print $1}')" >> "$manifest"
printf 'PASS WI-1907 xzy86 fixture set: manifest=%s\n' "$manifest"
