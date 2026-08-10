#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
base=${2:-$root/docs/avx-validation/wi-1904-three-way-a7d8dad3}
output=${3:-$base/wi-1904-test-report-raw.md}

emit_text()
{
    local title=$1
    local file=$2
    printf '\n## %s\n\nfile: `%s`\n\n```text\n' "$title" "$file"
    sed -n '1,320p' "$file"
    printf '```\n'
}

emit_hex()
{
    local title=$1
    local file=$2
    printf '\n### %s\n\nfile: `%s`\n\n```text\n' "$title" "$file"
    od -An -tx1 -v "$file" | tr -d ' \n'
    printf '\n```\n'
}

{
    printf '# WI-1904 actual three-way transcript\n\n'
    emit_text 'build output' "$base/wi-1904-build.log"
    emit_text 'xzy86 native build script' "$root/tests/integration/build-latx-avx-wi1904-xzy86.sh"
    emit_text 'three-way adapter script' "$root/tests/integration/build-latx-avx-wi1904-three-way-adapter.sh"
    emit_text 'xzy86 native build and run output' "$base/x86-native.log"
    emit_text 'xzy86 native manifest' "$base/x86-native/native-manifest.tsv"
    for file in "$base"/x86-native/*.faults; do
        emit_text "native faults $(basename "$file")" "$file"
    done
    for file in "$base"/x86-native/*.native; do
        emit_hex "native stdout $(basename "$file")" "$file"
    done

    for case_dir in \
        vmovhpd-default vmovhpd-fault-load vmovhpd-fault-store \
        vmovlpd-default vmovlpd-fault-load vmovlpd-fault-store; do
        case_name=${case_dir#*-}
        mnemonic=${case_dir%%-*}
        dir=$base/$case_dir/$mnemonic
        emit_text "$mnemonic $case_name JSON" "$base/$case_dir/$mnemonic.json"
        emit_text "$mnemonic $case_name runner output" "$base/$case_dir.run.log"
        emit_text "$mnemonic $case_name LSX GDB log" "$dir/lsx.gdb.log"
        for backend in x86 lasx lsx; do
            emit_optional=""
            if [ -f "$dir/$backend.stdout" ]; then
                emit_hex "$mnemonic $case_name $backend stdout" "$dir/$backend.stdout"
            fi
            if [ -f "$dir/$backend.stderr" ]; then
                emit_text "$mnemonic $case_name $backend stderr" "$dir/$backend.stderr"
            fi
            if [ -f "$dir/$backend.status" ]; then
                emit_text "$mnemonic $case_name $backend status" "$dir/$backend.status"
            fi
        done
    done
} > "$output"

printf 'generated %s (%s bytes)\n' "$output" "$(wc -c < "$output")"
