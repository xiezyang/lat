#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
python3 - "$root" <<'PY'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
source = (root / "target/i386/latx/translator/tr-avx-mov.c").read_text()
dispatch = (root / "target/i386/latx/translator/translate.c").read_text()
manifest = json.loads((root / "tests/integration/latx-avx-opt-only-manifest.json").read_text())
entry = next(item for item in manifest["entries"] if item["mnemonic"] == "vmovntdqa")
assert entry["runner"] == "tests/integration/test-latx-avx-wi1907-fixtures.sh"
assert entry["coverage_status"] == "existing_fixture"
for file_name in entry["source_files"]:
    assert (root / file_name).is_file()

start = source.index("bool translate_vmovntdqa(")
end = source.index("bool translate_vmovntdqa_lsx(", start)
lasx = source[start:end]
start = end
end = source.index("bool translate_vmovsd_lsx(", start)
lsx = source[start:end]
assert "la_xv" in lasx
assert "load_v128_from_ir1_mem_exact" in lsx
assert "load_v256_from_ir1_mem_exact" in lsx
assert "clear_ymm_high128_shadow" in lsx
assert "store_ymm_high128_shadow" in lsx
assert "la_xv" not in lsx

context_start = dispatch.index("void translate_context_init")
context_end = dispatch.index("\nbool ir1_translate", context_start)
context = dispatch[context_start:context_end]
assert "TRANS_FUNC_GEN(VMOVNTDQA, vmovntdqa)" in dispatch
assert "dt_X86_INS_VMOVNTDQA" in context
assert "translate_vmovntdqa_lsx" in context
assert "!option_enable_lasx || !option_avx_cpuid" in context

assembly = (root / "tests/integration/latx-avx-single-vmovntdqa.S").read_text()
assert assembly.count("vmovntdqa ") == 4
assert "vmovntdqa xmm0, xmmword ptr" in assembly
assert "vmovntdqa ymm0, ymmword ptr" in assembly
assert "fault_xmm" in assembly and "fault_ymm" in assembly
c_source = (root / "tests/integration/latx-avx-single-vmovntdqa.c").read_text()
assert "LATX_SYS_MPROTECT" in c_source
assert "output + 64" in c_source and "output + 96" in c_source
print("PASS WI-1907 source audit: no overlap, existing LSX path, LASX boundary, XMM/YMM/cache-hint/fault fixture")
PY
