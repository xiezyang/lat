#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
output=${2:-$root/docs/avx-validation/wi-1890-test-report-raw-32301813.generated.md}
build_log=$root/docs/avx-validation/wi-1890-alignment-fix-build.log
dqa=$root/docs/avx-validation/wi-1896-alignment-32301813
dqu=$root/docs/avx-validation/wi-1898-page-32301813
vmovsd=$root/docs/avx-validation/wi-1890-fault-record-32301813-fixed

emit_text()
{
    local title=$1
    local file=$2
    printf '\n## %s\n\n' "$title"
    printf 'file: `%s`\n\n```text\n' "$file"
    if [ -s "$file" ]; then
        sed -n '1,240p' "$file"
    fi
    printf '```\n'
}

emit_optional_text()
{
    local title=$1
    local file=$2
    if [ -f "$file" ]; then
        emit_text "$title" "$file"
    fi
}

emit_hex()
{
    local title=$1
    local file=$2
    printf '\n### %s\n\n' "$title"
    printf 'file: `%s`\n\n```text\n' "$file"
    od -An -tx1 -v "$file" | tr -d ' \n'
    printf '\n```\n'
}

emit_three_way_case()
{
    local suite=$1
    local case_name=$2
    local mnemonic=$3
    local dir=$suite/$case_name/$mnemonic

    emit_text "$mnemonic $case_name JSON" "$suite/$case_name/$mnemonic.json"
    emit_optional_text "$mnemonic $case_name x86 command" "$dir/x86.command"
    emit_optional_text "$mnemonic $case_name x86 status" "$dir/x86.status"
    emit_hex "$mnemonic $case_name x86 stdout" "$dir/x86.stdout"
    emit_text "$mnemonic $case_name x86 stderr" "$dir/x86.stderr"
    emit_optional_text "$mnemonic $case_name LASX command" "$dir/lasx.command"
    emit_optional_text "$mnemonic $case_name LASX status" "$dir/lasx.status"
    emit_hex "$mnemonic $case_name LASX stdout" "$dir/lasx.stdout"
    emit_text "$mnemonic $case_name LASX stderr" "$dir/lasx.stderr"
    emit_optional_text "$mnemonic $case_name LSX GDB command/log" "$dir/lsx.gdb.log"
    emit_optional_text "$mnemonic $case_name LSX status" "$dir/lsx.status"
    emit_hex "$mnemonic $case_name LSX stdout" "$dir/lsx.stdout"
    emit_text "$mnemonic $case_name LSX stderr" "$dir/lsx.stderr"
}

{
    printf '# WI-1890 actual test transcript\n\n'
    printf 'Generated from saved command, status, stdout, stderr, GDB and JSON files.\n'
    printf 'No result, hash or exit value is entered separately in this report.\n'
    emit_text 'build command output' "$build_log"
    emit_text 'source and binary SHA-256 files' "$root/docs/avx-validation/wi-1890-signal-fix-32301813-source-sha256sums.txt"

    for case_name in ymm-store-u ymm-store-cross; do
        emit_three_way_case "$dqa" "$case_name" vmovdqa
    done
    for case_name in xmm-load-page xmm-store-page ymm-load-page ymm-store-page; do
        emit_three_way_case "$dqu" "$case_name" vmovdqu
    done

    for case_name in load-cross-1 load-cross-7 store-cross-1 store-cross-7; do
        dir=$vmovsd/$case_name
        emit_text "VMOVSD $case_name LASX command" "$dir/lasx.command"
        emit_text "VMOVSD $case_name LASX status" "$dir/lasx.status"
        emit_hex "VMOVSD $case_name LASX stdout" "$dir/lasx.stdout"
        emit_text "VMOVSD $case_name LASX stderr" "$dir/lasx.stderr"
        emit_text "VMOVSD $case_name LSX command" "$dir/lsx.command"
        emit_text "VMOVSD $case_name LSX GDB output" "$dir/lsx.gdb"
        emit_text "VMOVSD $case_name LSX status" "$dir/lsx.status"
        emit_hex "VMOVSD $case_name LSX stdout" "$dir/lsx.stdout"
        emit_text "VMOVSD $case_name LSX stderr" "$dir/lsx.stderr"
    done

    emit_text 'VMOVSD result table' "$vmovsd/results.tsv"
    emit_text 'VMOVSD SHA-256 manifest' "$vmovsd/SHA256SUMS"
} > "$output"

printf 'generated %s (%s bytes)\n' "$output" "$(wc -c < "$output")"
