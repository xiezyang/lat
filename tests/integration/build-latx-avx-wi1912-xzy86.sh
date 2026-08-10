#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 3 ]]; then
    echo "usage: $0 [REMOTE_HOST] [REMOTE_DIR] [LOCAL_OUTPUT_DIR]" >&2
    exit 2
fi

remote_host=${1:-xzy86}
remote_dir=${2:-/tmp/latx-avx-wi1912}
local_output_dir=${3:-$(mktemp -d /tmp/wi-1912-x86-XXXXXX)}
case "$local_output_dir" in
/tmp/*) ;;
*) echo "local output must be under /tmp: $local_output_dir" >&2; exit 2 ;;
esac

script_dir=$(cd "$(dirname "$0")" && pwd)
source_file="$script_dir/latx-avx-wi1912-x86.c"
[[ -f "$source_file" ]] || { echo "FAIL missing fixture: $source_file" >&2; exit 2; }
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi

cases=(
    vbroadcastf128 vbroadcasti128 vbroadcastsd vbroadcastss
    vpbroadcastb vpbroadcastd vpbroadcastq vpbroadcastw
    vextractf128 vextracti128 vextractps
    vinsertf128 vinserti128 vinsertps
    vpextrb vpextrd vpextrq vpextrw
    vpinsrb vpinsrd vpinsrw
)

ssh "${ssh_args[@]}" "$remote_host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$source_file" "$remote_host:$remote_dir/latx-avx-wi1912-x86.c"
ssh "${ssh_args[@]}" "$remote_host" bash -s -- "$remote_dir" <<'REMOTE'
set -euo pipefail
remote_dir=$1
cases='vbroadcastf128 vbroadcasti128 vbroadcastsd vbroadcastss
vpbroadcastb vpbroadcastd vpbroadcastq vpbroadcastw
vextractf128 vextracti128 vextractps
vinsertf128 vinserti128 vinsertps
vpextrb vpextrd vpextrq vpextrw
vpinsrb vpinsrd vpinsrw'
docker run --rm --platform linux/amd64 \
    -e WI1912_CASES="$cases" -e WI1912_REMOTE_DIR="$remote_dir" \
    -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
        printf "instruction\texit\tbinary_sha256\tstdout_sha256\n" > /work/manifest
        index=0
        for mnemonic in $WI1912_CASES; do
            binary="/tmp/wi-1912-$mnemonic"
            build_log="/work/$mnemonic.build.log"
            run_stdout="/work/$mnemonic.stdout"
            run_stderr="/work/$mnemonic.stderr"
            run_status="/work/$mnemonic.exit"
            printf "%s\n" \
                "gcc -std=c11 -O0 -mavx2 -DWI1912_CASE=$index /work/latx-avx-wi1912-x86.c -o $binary" \
                >"/work/$mnemonic.command"
            set +e
            gcc -std=c11 -O0 -Wall -Wextra -Werror -mavx2 \
                -DWI1912_CASE="$index" /work/latx-avx-wi1912-x86.c \
                -o "$binary" >"$build_log" 2>&1
            build_status=$?
            set -e
            printf "%s\n" "$build_status" >"/work/$mnemonic.build.exit"
            test "$build_status" -eq 0
            objdump -d "$binary" >"/work/$mnemonic.objdump"
            set +e
            "$binary" >"$run_stdout" 2>"$run_stderr"
            run_status_value=$?
            set -e
            printf "%s\n" "$run_status_value" >"$run_status"
            test "$run_status_value" -eq 0
            test -s "$run_stdout"
            cp "$binary" "/work/$mnemonic.binary"
            printf "%s\t%s\t%s\t%s\n" "$mnemonic" "$run_status_value" \
                "$(sha256sum "$binary" | cut -d " " -f1)" \
                "$(sha256sum "$run_stdout" | cut -d " " -f1)" >> /work/manifest
            index=$((index + 1))
        done
        printf "remote_host=%s\n" "$(hostname)" >> /work/manifest
        printf "source=/work/latx-avx-wi1912-x86.c\n" >> /work/manifest
        printf "cases=%s\n" "$WI1912_CASES" >> /work/manifest
    '
REMOTE

mkdir -p "$local_output_dir"
scp "${scp_args[@]}" "$remote_host:$remote_dir/manifest" "$local_output_dir/"
for mnemonic in "${cases[@]}"; do
    scp "${scp_args[@]}" \
        "$remote_host:$remote_dir/$mnemonic.command" \
        "$remote_host:$remote_dir/$mnemonic.build.log" \
        "$remote_host:$remote_dir/$mnemonic.build.exit" \
        "$remote_host:$remote_dir/$mnemonic.objdump" \
        "$remote_host:$remote_dir/$mnemonic.binary" \
        "$remote_host:$remote_dir/$mnemonic.stdout" \
        "$remote_host:$remote_dir/$mnemonic.stderr" \
        "$remote_host:$remote_dir/$mnemonic.exit" \
        "$local_output_dir/"
done
printf 'PASS WI-1912 xzy86 baseline: %s\n' "$local_output_dir"
