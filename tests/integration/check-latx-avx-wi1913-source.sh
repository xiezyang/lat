#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
translator="$root/target/i386/latx/translator/tr-avx.c"
dispatch="$root/target/i386/latx/translator/translate.c"
header="$root/target/i386/latx/include/translate.h"
mnemonics=(
    vblendpd vblendps vblendvpd vblendvps vpalignr
    vpblendd vpblendvb vpblendw vperm2f128 vperm2i128
    vpermd vpermilpd vpermilps vpermpd vpermps vpermq
)

git -C "$root" diff --check

for mnemonic in "${mnemonics[@]}"; do
    grep -Fq "TRANS_FUNC_GEN(${mnemonic^^}" "$dispatch"
    grep -Fq "dt_X86_INS_${mnemonic^^}" "$dispatch"
done

start=$(rg -n '^        if \(!option_enable_lasx\) \{' "$dispatch" |
    head -n 1 | cut -d: -f1)
end=$(awk -v start="$start" 'NR > start && /^        \}/{print NR; exit}' "$dispatch")
lsx_block=$(sed -n "${start},${end}p" "$dispatch")
for mnemonic in "${mnemonics[@]}"; do
    printf '%s\n' "$lsx_block" | grep -Fq "dt_X86_INS_${mnemonic^^}"
done
if printf '%s\n' "$lsx_block" | grep -Eq 'la_xv'; then
    echo "FAIL LASX generator in WI-1913 registration block" >&2
    exit 1
fi

for mnemonic in "${mnemonics[@]}"; do
    case "$mnemonic" in
        vpermpd|vpermps) declaration=vpermpx_lsx ;;
        *) declaration=${mnemonic}_lsx ;;
    esac
    grep -Fq "TRANS_FUNC_DEF(${declaration})" "$header"
done

for function in \
    vblendpd vblendps vblendvpd vblendvps vpalignr vpblendd vpblendvb vpblendw \
    vperm2i128 vpermd vpermilpd vpermilps vpermpx vpermq; do
    body=$(awk -v fn="bool translate_${function}_lsx" \
        '$0 ~ fn {capture=1} capture {print} capture && /^}/ {exit}' "$translator")
    if printf '%s\n' "$body" | grep -Eq 'la_xv'; then
        echo "FAIL LASX instruction in ${function}_lsx" >&2
        exit 1
    fi
done

grep -Fq 'load_avx_lsx_operand' "$translator"
grep -Fq 'store_avx_lsx_result' "$translator"
grep -Fq 'la_vbsrl_v' "$translator"
grep -Fq 'la_vbitsel_v' "$translator"
grep -Fq 'translate_vpermute_w_dynamic_lsx' "$translator"

fixture_dir=$(python3 "$root/tests/integration/generate-latx-avx-wi1913-fixtures.py")
trap 'rm -rf "$fixture_dir"' EXIT
python3 - "$fixture_dir" <<'PY'
import json
import pathlib
import sys

directory = pathlib.Path(sys.argv[1])
manifest = json.loads((directory / "manifest.json").read_text())
assert len(manifest["mnemonics"]) == 16
ymm_only = {"vperm2f128", "vperm2i128", "vpermd", "vpermpd", "vpermps", "vpermq"}
for entry in manifest["mnemonics"]:
    source = pathlib.Path(entry["source"])
    assert source.is_file()
    text = source.read_text()
    assert text.count(f"{entry['mnemonic']} ") >= 4
    assert "ymm" in text
    if entry["mnemonic"] not in ymm_only:
        assert "xmm" in text
    assert "input_b" in text
print("PASS WI-1913 generated x86 fixture manifest: 16 mnemonics")
PY

echo "PASS WI-1913 source, dispatch, LASX preservation and fixture checks"
