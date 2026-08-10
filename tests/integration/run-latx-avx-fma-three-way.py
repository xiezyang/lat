#!/usr/bin/env python3
"""Run all 60 FMA fixtures through the existing three-way recorder."""

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "tests/integration/latx-avx-opt-only-manifest.json"
RUNNER = ROOT / "tests/integration/run-latx-avx-three-way.py"
BUILD = ROOT / "tests/integration/build-latx-avx-fma-xzy86.sh"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--latx", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--remote-host", default="xzy86")
    parser.add_argument("--ssh-config")
    parser.add_argument("--execute", action="store_true")
    args = parser.parse_args()
    entries = [entry["mnemonic"] for entry in json.loads(MANIFEST.read_text())["entries"]
               if entry["category"] == "fma"]
    cases = ("reference", "special", "rounding")
    summary = []
    for mnemonic in entries:
        for case in cases:
            output = args.output_dir / mnemonic / case
            command = ["python3", str(RUNNER), "--manifest", str(MANIFEST),
                       "--latx", str(args.latx), "--build-script", str(BUILD),
                       "--remote-host", args.remote_host, "--mnemonic", mnemonic,
                       "--case", case, "--output-dir", str(output)]
            if args.ssh_config:
                command += ["--ssh-config", args.ssh_config]
            command.append("--execute" if args.execute else "--plan")
            completed = subprocess.run(command, cwd=ROOT, check=False)
            summary.append({"mnemonic": mnemonic, "case": case,
                            "exit_status": completed.returncode,
                            "output_dir": str(output)})
            if completed.returncode != 0 and args.execute:
                (args.output_dir).mkdir(parents=True, exist_ok=True)
                (args.output_dir / "summary.json").write_text(
                    json.dumps(summary, indent=2) + "\n")
                return completed.returncode
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "summary.json").write_text(
        json.dumps(summary, indent=2) + "\n")
    print(f"PASS FMA three-way schedule: {len(entries)} mnemonics x {len(cases)} cases")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
