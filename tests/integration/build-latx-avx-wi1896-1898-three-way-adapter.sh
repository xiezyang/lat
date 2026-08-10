#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
    echo "usage: $0 MNEMONIC REMOTE_HOST REMOTE_DIR LOCAL_OUTPUT" >&2
    exit 2
fi

mnemonic=$1
remote_host=$2
remote_dir=$3
local_output=$4
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

case "$mnemonic" in
    vmovdqa)
        exec bash "$root/tests/integration/build-latx-avx-wi1896-xzy86.sh" \
            "$remote_host" "$remote_dir" "$local_output"
        ;;
    vmovdqu)
        exec bash "$root/tests/integration/build-latx-avx-wi1898-xzy86.sh" \
            "$remote_host" "$remote_dir" "$local_output"
        ;;
    *)
        echo "FAIL unsupported mnemonic: $mnemonic" >&2
        exit 2
        ;;
esac
