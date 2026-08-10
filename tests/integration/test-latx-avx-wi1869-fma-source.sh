#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
manifest=$root/tests/integration/latx-avx-opt-only-manifest.json
fma=$root/target/i386/latx/translator/tr-avx-fma.c
dispatch=$root/target/i386/latx/translator/translate.c

python3 "$root/tests/integration/generate-latx-avx-fma-fixtures.py" --check
python3 "$root/tests/integration/generate-latx-avx-single-mnemonic.py" \
    check --manifest "$manifest"

python3 - "$manifest" <<'PY'
import json
import sys

manifest = json.load(open(sys.argv[1]))
entries = [entry for entry in manifest["entries"] if entry["category"] == "fma"]
assert len(entries) == 60
assert all(entry["coverage_status"] == "existing_fixture" for entry in entries)
assert all(len(entry["source_files"]) == 2 and entry["runner"] for entry in entries)
assert {entry["mnemonic"][-2:] for entry in entries} == {"ps", "pd", "ss", "sd"}
print("PASS WI-1869 manifest: 60 FMA entries with independent fixtures")
PY

if grep -Eq 'la_xv|la_xvre|la_xvf' "$fma"; then
    echo "FAIL FMA LSX source contains LASX generator" >&2
    exit 1
fi
for name in vfmaddxxxpd vfmaddxxxps vfmaddxxxsd vfmaddxxxss \
    vfmsubxxxpd vfmsubxxxps vfmsubxxxsd vfmsubxxxss \
    vfnmaddxxxpd vfnmaddxxxps vfnmaddxxxsd vfnmaddxxxss \
    vfnmsubxxxpd vfnmsubxxxps vfnmsubxxxsd vfnmsubxxxss \
    vfmaddsubxxxpd vfmaddsubxxxps vfmsubaddxxxpd vfmsubaddxxxps; do
    grep -Fq "LSX_FMA_WRAPPERS(${name}," "$fma" || {
        echo "FAIL missing translate_${name}_lsx" >&2
        exit 1
    }
done

count=$(sed -n '/#define LATX_AVX_FMA_LSX_REGISTER/,/#undef LATX_AVX_FMA_LSX_REGISTER/p' \
    "$dispatch" | grep -c '^        LATX_AVX_FMA_LSX_REGISTER(')
[ "$count" -eq 60 ] || {
    echo "FAIL FMA LSX registration count: $count" >&2
    exit 1
}
grep -Fq 'bool translate_vfmaddxxxpd(IR1_INST * pir1)' \
    "$root/target/i386/latx/translator/tr-avx.c"
grep -Fq 'bool translate_vfmaddsubxxxps(IR1_INST * pir1)' \
    "$root/target/i386/latx/translator/tr-simd-fma.c"
echo "PASS WI-1869 FMA source: LASX paths retained and 60 LSX registrations centralized"
