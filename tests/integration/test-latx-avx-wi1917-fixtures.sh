#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
output_dir=${1:-$(mktemp -d /tmp/wi1917-x86.XXXXXX)}
remote_host=${2:-xzy86}
remote_dir=${3:-/tmp/latx-avx-wi1917-$(date +%s)-$$}
case "$output_dir" in /tmp/*) ;; *) exit 2 ;; esac
case "$remote_dir" in /tmp/*) ;; *) exit 2 ;; esac
script_dir=$root/tests/integration
python3 "$script_dir/generate-latx-avx-wi1917-fixtures.py"
bash "$script_dir/test-latx-avx-wi1917-source.sh" "$root"
bash -n "$script_dir/check-latx-avx-wi1917-mnemonic.sh" "$script_dir/build-latx-avx-wi1917-xzy86.sh" "$script_dir/test-latx-avx-wi1917-source.sh" "$script_dir/test-latx-avx-wi1917-fixtures.sh"
bash "$script_dir/build-latx-avx-wi1917-xzy86.sh" "$remote_host" "$remote_dir" "$output_dir"
manifest=$output_dir/native-manifest.tsv
printf 'instruction\tnormal_stdout_bytes\tbinary_sha256\tnormal_stdout_sha256\tnormal_status\tfault_status\tnormal_stderr_sha256\tfault_stderr_sha256\n' > "$manifest"
mnemonics=$(python3 "$script_dir/generate-latx-avx-wi1917-fixtures.py" | sed -n 's/^mnemonics=//p')
for m in $mnemonics; do
    stem=$output_dir/latx-avx-single-$m
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$m" \
        "$(wc -c < "$stem.native")" "$(sha256sum "$stem.static" | awk '{print $1}')" \
        "$(sha256sum "$stem.native" | awk '{print $1}')" "$(cat "$stem.normal.status")" \
        "$(cat "$stem.fault.status")" "$(sha256sum "$stem.native.stderr" | awk '{print $1}')" \
        "$(sha256sum "$stem.fault.stderr" | awk '{print $1}')" >> "$manifest"
done
[ "$(awk 'NR > 1 {n++} END {print n + 0}' "$manifest")" -eq 8 ]
printf 'PASS WI-1917 xzy86 fixture set count=8 manifest=%s\n' "$manifest"
