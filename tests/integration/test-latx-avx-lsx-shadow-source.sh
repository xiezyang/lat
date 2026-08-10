#!/usr/bin/env bash

set -euo pipefail

repo_root="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"
source_file="$repo_root/target/i386/latx/translator/tr-opnd-process.c"
header_file="$repo_root/target/i386/latx/include/lsenv.h"

helper_block="$(awk '/^static IR2_OPND get_ymm_high128_shadow_addr/{found = 1} found{print}' "$source_file")"

for symbol in \
    load_ymm_high128_shadow \
    store_ymm_high128_shadow \
    clear_ymm_high128_shadow \
    clear_all_ymm_high128_shadows; do
    if ! grep -q "$symbol" <<<"$helper_block"; then
        echo "FAIL missing helper: $symbol" >&2
        exit 1
    fi
done

if grep -Eq '\bla_xv' <<<"$helper_block"; then
    echo "FAIL YMM shadow helpers contain LASX instructions" >&2
    exit 1
fi

for required in la_vld la_vst la_vxor_v ra_free_temp; do
    if ! grep -q "$required" <<<"$helper_block"; then
        echo "FAIL missing LSX shadow operation: $required" >&2
        exit 1
    fi
done

if ! grep -q 'cpu->ymmh_regs\[i\]' "$header_file"; then
    echo "FAIL ymmh_regs offset helper is missing" >&2
    exit 1
fi

echo "PASS YMM high-128 shadow helpers use LSX only"
