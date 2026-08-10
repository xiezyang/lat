#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
runner=$script_dir/run-latx-avx-three-way.py
manifest=$script_dir/latx-avx-opt-only-manifest.json

python3 "$runner" --self-test
python3 "$runner" --manifest "$manifest" --plan >"${TMPDIR:-/tmp}/latx-avx-three-way-plan.json"
python3 - "${TMPDIR:-/tmp}/latx-avx-three-way-plan.json" <<'PY'
import json
import sys

plan = json.load(open(sys.argv[1]))
assert plan["manifest_summary"]["entry_count"] == 486
assert plan["manifest_summary"]["unique_mnemonic_count"] == 485
assert len(plan["entries"]) == 485
assert plan["counts"]["fixture_incomplete"] >= 400
assert any(item["status"] == "shared_fixture_conflict"
           and item["mnemonic"] == "vpsrlq" for item in plan["entries"])
print("PASS three-way runner manifest plan: 486 entries / 485 mnemonics")
PY
