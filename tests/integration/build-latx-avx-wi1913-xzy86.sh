#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT_DIR]" >&2
    exit 2
fi

remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1913-x86}
local_output=${3:-$(mktemp -d /tmp/wi-1913-x86-XXXXXX)}
case "$local_output" in
/tmp/*) ;;
*) echo "local output must be under /tmp: $local_output" >&2; exit 2 ;;
esac

root=$(cd "$(dirname "$0")/../.." && pwd)
script_dir="$root/tests/integration"
fixture_dir=$(python3 "$script_dir/generate-latx-avx-wi1913-fixtures.py")
trap 'rm -rf "$fixture_dir"' EXIT HUP INT TERM
mkdir -p "$local_output"

cases=(
    vblendpd vblendps vblendvpd vblendvps vpalignr
    vpblendd vpblendvb vpblendw vperm2f128 vperm2i128
    vpermd vpermilpd vpermilps vpermpd vpermps vpermq
)

ssh "$remote_host" mkdir -p "$remote_dir"
scp "$script_dir/latx-avx-wi1913-runner.c" "$fixture_dir"/*.S \
    "$remote_host:$remote_dir/"
ssh "$remote_host" bash -s -- "$remote_dir" <<'REMOTE'
set -euo pipefail
remote_dir=$1
cases='vblendpd vblendps vblendvpd vblendvps vpalignr
vpblendd vpblendvb vpblendw vperm2f128 vperm2i128
vpermd vpermilpd vpermilps vpermpd vpermps vpermq'
printf 'instruction\texit\tbinary_sha256\tstdout_sha256\n' >"$remote_dir/manifest"
for mnemonic in $cases; do
    case_count=4
    case "$mnemonic" in
        vblendpd|vblendps|vblendvpd|vblendvps|vpalignr|vpblendd|vpblendvb|vpblendw|vpermilpd|vpermilps)
            case_count=6 ;;
    esac
    bytes=$((case_count * 32))
    binary="$remote_dir/$mnemonic.binary"
    build_log="$remote_dir/$mnemonic.build.log"
    stdout="$remote_dir/$mnemonic.stdout"
    stderr="$remote_dir/$mnemonic.stderr"
    printf '%s\n' "gcc -std=c11 -O0 -Wall -Wextra -Werror -mavx2 -no-pie -DWI1913_SYMBOL=latx_avx_single_${mnemonic}_run -DWI1913_BYTES=$bytes /work/latx-avx-wi1913-runner.c /work/latx-avx-single-$mnemonic.S -o $binary" >"$remote_dir/$mnemonic.command"
    set +e
    docker run --rm --platform linux/amd64 -v "$remote_dir:/work" -w /work \
        -e MNEMONIC="$mnemonic" -e BYTES="$bytes" \
        latx-ci-baseline:latest bash -ceu '
            gcc -std=c11 -O0 -Wall -Wextra -Werror -mavx2 -no-pie \
                -DWI1913_SYMBOL=latx_avx_single_${MNEMONIC}_run \
                -DWI1913_BYTES=${BYTES} \
                /work/latx-avx-wi1913-runner.c \
                /work/latx-avx-single-${MNEMONIC}.S \
                -o /work/${MNEMONIC}.binary
            objdump -d /work/${MNEMONIC}.binary > /work/${MNEMONIC}.objdump
            /work/${MNEMONIC}.binary > /work/${MNEMONIC}.stdout \
                2> /work/${MNEMONIC}.stderr
        ' >"$build_log" 2>&1
    status=$?
    set -e
    printf '%s\n' "$status" >"$remote_dir/$mnemonic.exit"
    if [ "$status" -eq 0 ]; then
        printf '%s\t%s\t%s\t%s\n' "$mnemonic" "$status" \
            "$(sha256sum "$binary" | cut -d ' ' -f1)" \
            "$(sha256sum "$stdout" | cut -d ' ' -f1)" >>"$remote_dir/manifest"
    fi
done
printf 'remote_host=%s\n' "$(hostname)" >>"$remote_dir/manifest"
printf 'fixture_cases=%s\n' "$cases" >>"$remote_dir/manifest"
REMOTE

scp "$remote_host:$remote_dir/manifest" "$local_output/"
for mnemonic in "${cases[@]}"; do
    scp "$remote_host:$remote_dir/$mnemonic.command" \
        "$remote_host:$remote_dir/$mnemonic.build.log" \
        "$remote_host:$remote_dir/$mnemonic.exit" \
        "$remote_host:$remote_dir/$mnemonic.stdout" \
        "$remote_host:$remote_dir/$mnemonic.stderr" \
        "$remote_host:$remote_dir/$mnemonic.objdump" \
        "$remote_host:$remote_dir/$mnemonic.binary" "$local_output/"
done
printf 'WI-1913 xzy86 fixture output=%s\n' "$local_output"
