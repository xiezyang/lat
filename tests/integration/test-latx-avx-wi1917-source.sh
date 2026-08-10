#!/usr/bin/env bash
set -euo pipefail
root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
python3 - "$root" <<'PY'
import json, sys
from pathlib import Path
root = Path(sys.argv[1])
names = "vpgatherdd vpgatherdq vpgatherqd vpgatherqq vgatherdps vgatherdpd vgatherqps vgatherqpd".split()
assert len(names) == 8
manifest = json.loads((root / "tests/integration/latx-avx-opt-only-manifest.json").read_text())
for name in names:
    assert next(x for x in manifest["entries"] if x["mnemonic"] == name)
    asm = root / f"tests/integration/latx-avx-single-{name}.S"
    src = root / f"tests/integration/latx-avx-single-{name}.c"
    assert asm.is_file() and src.is_file()
    text = asm.read_text()
    expected = 3 if name in {"vpgatherqd", "vgatherqps"} else 5
    assert text.count(f"    {name} ") == expected
    assert ".Lfault:" in text and "gather_base" in text
print("PASS WI-1917 fixture audit: 8 gather fixtures, index/mask/fault coverage")
PY
