#!/usr/bin/env bash
set -euo pipefail

repo=${1:-.}
translate="$repo/target/i386/latx/translator/translate.c"
header="$repo/target/i386/latx/include/translate.h"

test -f "$translate"
test -f "$header"

python3 - "$translate" "$header" <<'PY'
import re
import sys
from pathlib import Path

translate = Path(sys.argv[1]).read_text()
header = Path(sys.argv[2]).read_text()
base = translate.index("void translate_context_init(void)")
end = translate.index("\n}\n#endif", base) + 2
block = translate[base:end]

gates = []
for match in re.finditer(r"if\s*\(\s*!option_enable_lasx\s*\)", block):
    opening = block.find("{", match.end())
    depth = 0
    for pos in range(opening, len(block)):
        if block[pos] == "{":
            depth += 1
        elif block[pos] == "}":
            depth -= 1
            if depth == 0:
                gates.append((opening, pos))
                break

def is_gated(pos):
    return any(start <= pos <= finish for start, finish in gates)

def line(pos):
    return translate.count("\n", 0, base + pos) + 1

direct = []
pattern = re.compile(
    r"translate_register_lsx\s*\(\s*dt_X86_INS_"
    r"([A-Z0-9]+)\s*,\s*([A-Za-z0-9_]+)\s*\)\s*;",
    re.S,
)
for match in pattern.finditer(block):
    direct.append((match.group(1), match.group(2), line(match.start()), is_gated(match.start())))

table_sizes = {
    "LATX_AVX_INTEGER_CMP_LSX_TABLE": 8,
    "LATX_AVX_INTEGER_SHIFT_LSX_TABLE": 15,
    "LATX_AVX_INTEGER_3OP_LSX_TABLE": 28,
    "LATX_AVX_INTEGER_REMAINING_3OP_LSX_TABLE": 8,
}
table_rows = []
for name, expected in table_sizes.items():
    table = re.search(
        r"#define\s+" + re.escape(name) + r"\(X\)\s*\\\n"
        r"(.*?)(?=\n/\*|\n#define|\n#ifdef|\n#endif)",
        header,
        re.S,
    )
    if not table:
        raise SystemExit(f"missing table {name}")
    rows = re.findall(r"X\(([^)]*)\)", table.group(1))
    if len(rows) != expected:
        raise SystemExit(f"{name}: expected {expected}, found {len(rows)}")
    for match in re.finditer(re.escape(name) + r"\s*\(\s*LATX_", block):
        table_rows.extend((name, i, line(match.start()), is_gated(match.start())) for i in range(expected))

fma = []
for match in re.finditer(
    r"LATX_AVX_FMA_LSX_REGISTER\(\s*([A-Z0-9]+)\s*,\s*([a-z0-9_]+)\s*\)",
    block,
):
    fma.append((match.group(1), match.group(2), line(match.start()), is_gated(match.start())))

# These 38 registrations were added by WI-1915 after the 144-entry audit was
# written. Keep them explicit so the checker covers the current source too.
wi1915 = {
    "VPAVGB", "VPAVGW", "VPMULDQ", "VPMULHUW", "VPMULHW", "VPMULLD",
    "VPMULLW", "VPMULUDQ", "VPSIGNB", "VPSIGND", "VPSIGNW", "VPABSB",
    "VPABSD", "VPABSW", "VPMADDUBSW", "VPMADDWD", "VPMULHRSW", "VPHADDD",
    "VPHADDSW", "VPHADDW", "VPHSUBD", "VPHSUBSW", "VPHSUBW", "VPHMINPOSUW",
    "VPMOVMSKB", "VPMOVSXBD", "VPMOVSXBQ", "VPMOVSXBW", "VPMOVSXDQ",
    "VPMOVSXWD", "VPMOVSXWQ", "VPMOVZXBD", "VPMOVZXBQ", "VPMOVZXBW",
    "VPMOVZXDQ", "VPMOVZXWD", "VPMOVZXWQ", "VPSADBW",
}

first_batch = {
    "VMOVDQA", "VMOVDQU", "VMOVUPD", "VMOVUPS", "VMOVAPD", "VMOVAPS",
    "VMOVDDUP", "VMOVSHDUP", "VMOVSLDUP", "VMOVSS", "VMOVD", "VMOVQ",
    "VMOVSD", "VMOVMSKPD", "VMOVMSKPS", "VLDDQU", "VMASKMOVPD", "VMASKMOVPS",
}

wi1915_tables = {"LATX_AVX_INTEGER_REMAINING_3OP_LSX_TABLE": 8}
legacy_table_rows = sum(
    expected for name, expected in table_sizes.items() if name not in wi1915_tables
)
if sum(row[0] in wi1915 for row in direct) != 30:
    raise SystemExit("WI-1915 direct registration inventory changed")
if sum(expected for expected in wi1915_tables.values()) != 8:
    raise SystemExit("WI-1915 table registration inventory changed")

# The WI title names the 144 legacy entries. The current tree also contains
# 38 WI-1915 entries added after that audit: 30 direct and 8 table rows.
# Before this fix, the source audit found 33 direct legacy entries outside a
# gate; the other 119 direct legacy entries were already gated.
legacy_scope = 33 + len(fma) + legacy_table_rows
wi1915_scope = 30 + 8
if legacy_scope != 144 or wi1915_scope != 38:
    raise SystemExit(
        f"scope inventory changed: legacy={legacy_scope}, wi1915={wi1915_scope}"
    )
initial_ungated = legacy_scope + wi1915_scope
if len(first_batch) != 18 or not first_batch.issubset(set(wi1915) | {
    row[0] for row in direct
}):
    raise SystemExit("first dispatch batch inventory changed")
remaining_after_first = initial_ungated - len(first_batch)
if initial_ungated != 182 or remaining_after_first != 164:
    raise SystemExit(
        f"checkpoint inventory changed: initial={initial_ungated}, "
        f"remaining_after_first={remaining_after_first}"
    )

ungated_direct = [row for row in direct if not row[3]]
ungated_fma = [row for row in fma if not row[3]]
ungated_tables = [row for row in table_rows if not row[3]]
ungated = len(ungated_direct) + len(ungated_fma) + len(ungated_tables)
legacy_ungated = (
    sum(row[0] not in wi1915 for row in ungated_direct)
    + len(ungated_fma)
    + sum(row[0] not in wi1915_tables for row in ungated_tables)
)
wi1915_ungated = (
    sum(row[0] in wi1915 for row in ungated_direct)
    + sum(row[0] in wi1915_tables for row in ungated_tables)
)

print("WI-1925 dispatch audit")
print(f"initial unprotected entries: {initial_ungated}")
print(f"first batch moved under option_enable_lasx=0: {len(first_batch)}")
print(f"remaining after first batch: {remaining_after_first}")
print(f"pre-fix legacy scope: {legacy_scope}")
print(f"pre-fix WI-1915 additions: {wi1915_scope}")
print(f"direct registrations: {len(direct)}")
print(f"FMA registrations: {len(fma)}")
print(f"macro-expanded registrations: {len(table_rows)}")
print(f"ungated legacy entries: {legacy_ungated}")
print(f"ungated WI-1915 entries: {wi1915_ungated}")
print(f"ungated total: {ungated}")
for opcode, function, source_line, _ in ungated_direct + ungated_fma:
    print(f"UNGATED\tdirect\t{opcode}\t{function}\tline={source_line}")
for table, index, source_line, _ in ungated_tables:
    print(f"UNGATED\t{table}\trow={index}\tline={source_line}")

if ungated:
    raise SystemExit("LSX registration exists outside option_enable_lasx=0 gate")
if legacy_ungated != 0 or wi1915_ungated != 0:
    raise SystemExit("registration gate audit failed")
# VMOVNTDQ/VMOVNTPD/VMOVNTPS were added after the original 319-entry
# inventory and are part of the gated AVX move registrations.
if len(direct) + len(fma) + len(table_rows) != 322:
    raise SystemExit("unexpected registration inventory; review scope before changing code")
if "TRANS_FUNC_GEN(VPAND, vpand)" not in translate:
    raise SystemExit("LASX generic VPAND table entry is missing")
if "TRANS_FUNC_GEN(VPCLMULQDQ, vpclmulqdq)" not in translate:
    raise SystemExit("LASX generic VPCLMULQDQ table entry is missing")
print("PASS WI-1925: 144 legacy + 38 WI-1915 registrations are gated; LASX generic anchors remain")
PY
