#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 X86_ARTIFACT_ROOT THREE_WAY_OUTPUT_ROOT" >&2
    exit 2
fi

artifact_root=$1
run_root=$2

python3 - "$artifact_root" "$run_root" <<'PY'
from pathlib import Path
import csv
import re
import sys

artifact_root = Path(sys.argv[1])
run_root = Path(sys.argv[2])
names = (
    "vpcmpeqb", "vpcmpeqw", "vpcmpeqd", "vpcmpeqq",
    "vpcmpgtb", "vpcmpgtw", "vpcmpgtd", "vpcmpgtq",
)

with (run_root / "results.tsv").open(newline="") as stream:
    rows = {row["name"]: row for row in csv.DictReader(stream, delimiter="\t")}

for name in names:
    row = rows[name]
    assert row["x86_rc"] == "0", (name, row)
    assert row["lasx_rc"] == "0", (name, row)
    assert row["lsx_rc"] == "0", (name, row)
    assert row["lsx_signal"] == "0", (name, row)
    assert row["option_enable_lasx"] == "0", (name, row)

    expected = (artifact_root / "results" / name / "stdout.bin").read_bytes()
    lasx = (run_root / name / "lasx.stdout").read_bytes()
    lsx = (run_root / name / "lsx.stdout").read_bytes()
    assert lsx == expected, name

    record_count = len(expected) // 32
    assert len(expected) % 32 == 0, name
    assert lasx != expected, name
    low_equal = sum(
        expected[offset:offset + 16] == lasx[offset:offset + 16]
        for offset in range(0, len(expected), 32)
    )
    high_different = sum(
        expected[offset + 16:offset + 32] != lasx[offset + 16:offset + 32]
        for offset in range(0, len(expected), 32)
    )
    assert low_equal == record_count, (name, low_equal, record_count)
    assert high_different == record_count, (name, high_different, record_count)
    assert (run_root / name / "lsx.stderr").read_bytes() == b""
    gdb_log = (run_root / name / "lsx.gdb").read_text(errors="replace")
    assert re.search(r"<option_enable_lasx>:\s+0$", gdb_log, re.MULTILINE)
    assert "exited normally" in gdb_log
    print(
        f"{name}: LSX=x86 {record_count}/{record_count} records; "
        f"LASX low={low_equal}/{record_count}, high-different="
        f"{high_different}/{record_count}; rc/signal=0/0; option=0"
    )

print("PASS WI-1840 LSX acceptance; LASX high-half deviations recorded")
PY
