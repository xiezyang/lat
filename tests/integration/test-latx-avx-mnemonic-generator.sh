#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
generator=$script_dir/generate-latx-avx-single-mnemonic.py
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

python3 "$generator" manifest --output "$tmpdir/manifest.json"
python3 "$generator" \
    --source "$script_dir/../../target/i386/latx/translator/translate.c" \
    check --manifest "$tmpdir/manifest.json"

summary=$(python3 "$generator" summary)
printf '%s\n' "$summary" | grep -Fq '"duplicate_mnemonics": {"vpaddq": 2}'

python3 "$generator" generate --mnemonic vpsrlq --output-dir "$tmpdir/vpsrlq"
"$tmpdir/vpsrlq/test-latx-avx-single-vpsrlq.sh" --help >/dev/null 2>&1 || true

python3 "$generator" generate --mnemonic vaddpd --output-dir "$tmpdir/vaddpd"
set +e
manual_output=$("$tmpdir/vaddpd/test-latx-avx-single-vaddpd.sh" 2>&1)
manual_status=$?
set -e
[ "$manual_status" -eq 77 ]
printf '%s\n' "$manual_output" | grep -Fq \
    'NEEDS_MANUAL_TEMPLATE mnemonic=vaddpd'

echo "PASS AVX mnemonic manifest and generator self-check"
