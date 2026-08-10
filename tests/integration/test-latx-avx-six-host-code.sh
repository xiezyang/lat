#!/usr/bin/env bash

set -euo pipefail

latx_binary="${1:-build64/latx-x86_64}"

if [[ ! -x "$latx_binary" ]]; then
    echo "FAIL LATX binary is not executable: $latx_binary" >&2
    exit 2
fi

symbols=(
    translate_vmovdqa_lsx
    translate_vmovdqu_lsx
    translate_vmovss_lsx
    translate_vextracti128_lsx
    translate_vpxor_lsx
    translate_vzeroupper_lsx
    load_ymm_high128_shadow
    store_ymm_high128_shadow
    clear_ymm_high128_shadow
    clear_all_ymm_high128_shadows
)

for symbol in "${symbols[@]}"; do
    dump="$(objdump -d --disassemble="$symbol" "$latx_binary")"

    if ! grep -q "<$symbol>" <<<"$dump"; then
        echo "FAIL symbol is missing from LATX binary: $symbol" >&2
        exit 1
    fi
    if grep -Eq '<la_xv[^>]*>' <<<"$dump"; then
        echo "FAIL $symbol can call a LASX instruction generator" >&2
        grep -E '<la_xv[^>]*>' <<<"$dump" >&2
        exit 1
    fi
done

echo "PASS compiled six-instruction translators cannot call la_xv generators"
