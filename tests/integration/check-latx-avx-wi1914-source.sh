#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
python3 - "$root" <<'PY'
from pathlib import Path
import sys

root = Path(sys.argv[1])
scope = [
    line.strip()
    for line in (root / "tests/integration/latx-avx-wi1914-mnemonics.txt").read_text().splitlines()
    if line.strip()
]
header = (root / "target/i386/latx/include/translate.h").read_text()
source = (root / "target/i386/latx/translator/tr-avx.c").read_text()
registration = (root / "target/i386/latx/translator/translate.c").read_text()

def brace_block(text, start):
    opening = text.index("{", start)
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    raise AssertionError("unterminated registration block")

registration_blocks = []
search_from = 0
marker = "if (!option_enable_lasx) {"
while True:
    try:
        registration_start = registration.index(marker, search_from)
    except ValueError:
        break
    registration_blocks.append(brace_block(registration, registration_start))
    search_from = registration_start + len(marker)
registration_block = "\n".join(registration_blocks)
missing = []
for mnemonic in scope:
    function = f"translate_{mnemonic}_lsx"
    if f"bool {function}" not in source:
        missing.append(f"function:{mnemonic}")
    if f"TRANS_FUNC_DEF({mnemonic}_lsx)" not in header:
        missing.append(f"declaration:{mnemonic}")
    if f"dt_X86_INS_{mnemonic.upper()}" not in registration_block:
        missing.append(f"registration:{mnemonic}")
    if function not in registration_block:
        missing.append(f"dispatch:{mnemonic}")
if missing:
    raise SystemExit("missing: " + ", ".join(missing))

lsx_start = source.rindex("typedef IR2_INST *(*avx_lsx_fp_binary_fn)")
lsx_block = source[lsx_start:]
if "la_xv" in lsx_block:
    raise SystemExit("new WI-1914 LSX block contains la_xv*")
for primitive in ("la_vfrecip_s", "la_vfrsqrt_s"):
    if primitive not in lsx_block:
        raise SystemExit(f"missing dedicated approximate primitive: {primitive}")
print(f"PASS WI-1914 source: {len(scope)} functions/declarations/dispatches")
PY
