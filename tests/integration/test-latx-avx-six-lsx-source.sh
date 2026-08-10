#!/usr/bin/env bash

set -euo pipefail

repo_root="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"

translator_dir="$repo_root/target/i386/latx/translator"
dispatch="$translator_dir/translate.c"

if grep -R -n 'if (1)' \
    "$translator_dir/tr-avx.c" "$translator_dir/tr-avx-mov.c" \
    "$translator_dir/tr-avx-shift.c" "$translator_dir/tr-avx-cvt.c" \
    "$translator_dir/tr-avx-cmp.c" "$translator_dir/tr-simd-cvt.c"; then
    echo "FAIL AVX translator contains unreachable if (1)" >&2
    exit 1
fi

for function in \
    vmovdqa vmovdqu vmovups vmovaps vmovddup vmovss vmovd vmovq vmovsd \
    vmulsd vdivsd vxorpd vpblendvb vpunpcklqdq vpsrlq \
    vcvtsd2ss vcvtsi2sd vpcmpeqq vcvttsd2si \
    vpand vpor vpxor vextracti128 vinserti128 vpbroadcastq \
    vucomisd vzeroupper vpinsrq; do
    if ! grep -R -q "translate_${function}_lsx" "$translator_dir"; then
        echo "FAIL missing translate_${function}_lsx" >&2
        exit 1
    fi
    if ! grep -q "translate_${function}_lsx" "$dispatch"; then
        echo "FAIL missing dispatch entry for ${function}" >&2
        exit 1
    fi
done

for function in \
    vmovdqa vmovdqu vmovups vmovaps vmovddup vmovss vmovd vmovq vmovsd \
    vmulsd vdivsd vxorpd vpblendvb vpunpcklqdq vpsrlq \
    vcvtsd2ss vcvtsi2sd vpcmpeqq vcvttsd2si \
    vpand vpor vpxor vextracti128 vinserti128 vpbroadcastq \
    vucomisd vzeroupper vpinsrq; do
    file=$(rg -l "translate_${function}_lsx" "$translator_dir" | head -1)
    body=$(awk -v signature="bool translate_${function}_lsx" '
        index($0, signature) > 0 {
            capture = 1
            depth = 0
            current = ""
        }
        capture {
            current = current $0 ORS
            opens = gsub(/\{/, "{")
            closes = gsub(/\}/, "}")
            depth += opens - closes
            if (depth == 0) {
                last = current
                capture = 0
            }
        }
        END { printf "%s", last }
    ' "$file")
    if printf '%s' "$body" | grep -Eq '\bla_xv'; then
        echo "FAIL ${function}_lsx source contains LASX generator" >&2
        exit 1
    fi
done

echo "PASS AVX LSX split and centralized dispatch source checks"
