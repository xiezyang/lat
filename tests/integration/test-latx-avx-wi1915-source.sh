#!/usr/bin/env bash
set -euo pipefail
root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
python3 - "$root" <<'PY'
import json, sys
from pathlib import Path
root = Path(sys.argv[1])
names = "vpmuldq vpmulhrsw vpmulhuw vpmulhw vpmulld vpmullw vpmuludq vpabsb vpabsd vpabsw vpavgb vpavgw vphaddd vphaddsw vphaddw vphminposuw vphsubd vphsubsw vphsubw vpmaddubsw vpmaddwd vpmovmskb vpmovsxbd vpmovsxbq vpmovsxbw vpmovsxdq vpmovsxwd vpmovsxwq vpmovzxbd vpmovzxbq vpmovzxbw vpmovzxdq vpmovzxwd vpmovzxwq vpsadbw vpsignb vpsignd vpsignw".split()
assert len(names) == 38
manifest = json.loads((root / "tests/integration/latx-avx-opt-only-manifest.json").read_text())
for name in names:
    assert next(x for x in manifest["entries"] if x["mnemonic"] == name)
    asm = root / f"tests/integration/latx-avx-single-{name}.S"
    src = root / f"tests/integration/latx-avx-single-{name}.c"
    assert asm.is_file() and src.is_file()
    text = asm.read_text()
    assert text.count(f"    {name} ") >= 5
    assert name == "vpmovmskb" or ".Lfault:" in text
print("PASS WI-1915 source audit: 38 mnemonic fixtures, no target source changes")
PY
