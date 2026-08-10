#!/usr/bin/env python3
"""Run WI-1839 probes against a supplied x86 artifact set.

The default mode only validates artifact inputs. Guest execution requires the
explicit --execute flag so an old LATX binary cannot be used accidentally.
Use --check-only to make the no-execution mode explicit in CI or handoff logs.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
from pathlib import Path


EXPECTED_ARTIFACT_MANIFEST_SHA256 = (
    "8ac9a38bbee3168b8d07e660f728cdce0ae44fc2a134c4d289927122474e77e0"
)

MNEMONICS = (
    "vpaddb", "vpaddd", "vpaddq", "vpaddsb", "vpaddsw", "vpaddusb",
    "vpaddusw", "vpaddw", "vpmaxsb", "vpmaxsd", "vpmaxsw", "vpmaxub",
    "vpmaxud", "vpmaxuw", "vpminsb", "vpminsd", "vpminsw", "vpminub",
    "vpminud", "vpminuw", "vpsubb", "vpsubd", "vpsubq", "vpsubsb",
    "vpsubsw", "vpsubusb", "vpsubusw", "vpsubw",
)

SOURCE_FILES = (
    "target/i386/latx/include/translate.h",
    "target/i386/latx/translator/tr-avx.c",
    "target/i386/latx/translator/translate.c",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_manifest(root: Path) -> list[dict[str, object]]:
    lines = (root / "manifest.tsv").read_text().splitlines()
    rows = []
    for line in lines:
        if not line or line.startswith("instruction\t"):
            continue
        fields = line.split("\t")
        if len(fields) != 9:
            raise ValueError(f"invalid manifest row: {line}")
        mnemonic, exit_code, stdout_bytes = fields[:3]
        rows.append(
            {
                "mnemonic": mnemonic,
                "native_exit_code": int(exit_code),
                "native_stdout_bytes": int(stdout_bytes),
                "native_binary_sha256": fields[3],
                "assembly_sha256": fields[4],
                "c_sha256": fields[5],
                "native_stdout_sha256": fields[6],
            }
        )
    if tuple(row["mnemonic"] for row in rows) != MNEMONICS:
        raise ValueError("manifest mnemonic order or contents differ from WI-1839")
    return rows


def validate_artifacts(root: Path, artifact_manifest_sha256: str) -> list[dict[str, object]]:
    manifest_file = root / "artifact-files.sha256"
    actual_manifest_sha256 = sha256(manifest_file)
    if actual_manifest_sha256 != artifact_manifest_sha256:
        raise ValueError(
            "artifact-files.sha256 hash mismatch: "
            f"expected {artifact_manifest_sha256}, got {actual_manifest_sha256}"
        )

    rows = parse_manifest(root)
    for row in rows:
        mnemonic = row["mnemonic"]
        if row["native_exit_code"] != 0 or row["native_stdout_bytes"] != 256:
            raise ValueError(f"native status/layout invalid: {mnemonic}")
        probe = root / f"latx-avx-single-{mnemonic}.static"
        stdout = root / "results" / mnemonic / "stdout.bin"
        status = root / "results" / mnemonic / "exit.status"
        if sha256(probe) != row["native_binary_sha256"]:
            raise ValueError(f"native binary hash mismatch: {mnemonic}")
        if sha256(stdout) != row["native_stdout_sha256"]:
            raise ValueError(f"native stdout hash mismatch: {mnemonic}")
        if stdout.stat().st_size != 256 or status.read_text().strip() != "0":
            raise ValueError(f"native artifact content mismatch: {mnemonic}")
    return rows


def source_hashes(repo_root: Path) -> dict[str, str]:
    return {relative: sha256(repo_root / relative) for relative in SOURCE_FILES}


def signal_for_returncode(returncode: int) -> int | None:
    if returncode < 0:
        return -returncode
    if 128 < returncode < 192:
        return returncode - 128
    return None


def run_command(argv: list[str], env: dict[str, str]) -> subprocess.CompletedProcess[bytes]:
    merged = os.environ.copy()
    merged.update(env)
    return subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          check=False, env=merged)


def write_run(path: Path, completed: subprocess.CompletedProcess[bytes]) -> dict[str, object]:
    path.write_bytes(completed.stdout)
    path.with_suffix(".stderr").write_bytes(completed.stderr)
    path.with_suffix(".status").write_text(f"{completed.returncode}\n")
    return {
        "exit_code": completed.returncode,
        "signal": signal_for_returncode(completed.returncode),
        "stdout_bytes": len(completed.stdout),
        "stdout_sha256": hashlib.sha256(completed.stdout).hexdigest(),
        "stderr_sha256": hashlib.sha256(completed.stderr).hexdigest(),
    }


def parse_gdb_guest_status(text: str, fallback: int) -> int:
    match = re.search(r"exited with code ([0-7]+)", text)
    if match:
        return int(match.group(1), 8)
    if "exited normally" in text:
        return 0
    signal_numbers = {
        "SIGILL": 4,
        "SIGBUS": 7,
        "SIGFPE": 8,
        "SIGSEGV": 11,
        "SIGABRT": 6,
        "SIGTRAP": 5,
    }
    match = re.search(r"(?:received signal|signal) (SIG[A-Z0-9]+)", text)
    if match and match.group(1) in signal_numbers:
        return 128 + signal_numbers[match.group(1)]
    return fallback


def option_address(latx: Path) -> str:
    completed = subprocess.run(["nm", "-g", str(latx)],
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               check=False)
    for line in completed.stdout.decode(errors="replace").splitlines():
        fields = line.split()
        if len(fields) >= 3 and fields[2] == "option_enable_lasx":
            return "0x" + fields[0]
    raise RuntimeError("option_enable_lasx symbol not found")


def compare_files(left: Path, right: Path) -> dict[str, object]:
    left_bytes = left.read_bytes()
    right_bytes = right.read_bytes()
    mismatch = []
    for offset, (left_byte, right_byte) in enumerate(zip(left_bytes, right_bytes)):
        if left_byte != right_byte:
            mismatch.append(
                {"offset": offset, "left": left_byte, "right": right_byte}
            )
            if len(mismatch) == 16:
                break
    return {
        "equal": left_bytes == right_bytes,
        "left_bytes": len(left_bytes),
        "right_bytes": len(right_bytes),
        "first_mismatches": mismatch,
    }


def classify_result(
    native: dict[str, object],
    lasx: dict[str, object],
    lsx: dict[str, object],
    lasx_compare: dict[str, object],
    lsx_compare: dict[str, object],
) -> str:
    if lsx["option_enable_lasx_readback"] != 0:
        return "lsx_option_unconfirmed"
    if lsx["gdb_exit_code"] != 0:
        return "platform_error"

    native_key = (native["exit_code"], native["stdout_sha256"])
    lasx_key = (lasx["exit_code"], lasx["stdout_sha256"])
    lsx_key = (lsx["exit_code"], lsx["stdout_sha256"])
    lasx_match = native_key == lasx_key and lasx_compare["equal"]
    lsx_match = native_key == lsx_key and lsx_compare["equal"]
    if lasx_match and lsx_match:
        return "pass_x86_stdout_status"
    if not lasx_match and not lsx_match:
        return "common_error"
    if not lasx_match:
        return "lasx_error"
    return "lsx_error"


def run_one(
    row: dict[str, object],
    artifact_root: Path,
    latx: Path,
    output_root: Path,
    source_digest: dict[str, str],
    option_addr: str,
) -> dict[str, object]:
    mnemonic = row["mnemonic"]
    probe = artifact_root / f"latx-avx-single-{mnemonic}.static"
    case_dir = output_root / mnemonic
    case_dir.mkdir(parents=True, exist_ok=True)
    native_stdout = artifact_root / "results" / mnemonic / "stdout.bin"
    native_status = artifact_root / "results" / mnemonic / "exit.status"

    lasx_env = {
        "LATX_AVX_CPUID": "1",
        "LATX_AOT": "0",
    }
    lasx = run_command([str(latx), str(probe)], lasx_env)
    lasx_record = write_run(case_dir / "lasx.stdout", lasx)

    lsx_stdout = case_dir / "lsx.stdout"
    lsx_stderr = case_dir / "lsx.stderr"
    gdb_log = case_dir / "lsx.gdb.log"
    run_text = (
        "run " + shlex.quote(str(probe)) + " > " + shlex.quote(str(lsx_stdout))
        + " 2> " + shlex.quote(str(lsx_stderr))
    )
    gdb_argv = [
        "gdb", "-q", "-batch", str(latx),
        "-ex", "set pagination off",
        "-ex", "set environment LATX_AVX_CPUID 1",
        "-ex", "set environment LATX_AOT 0",
        "-ex", "break translate_context_init",
        "-ex", run_text,
        "-ex", f"set {{int}}{option_addr} = 0",
        "-ex", f"x/wd {option_addr}",
        "-ex", "continue",
    ]
    gdb = subprocess.run(gdb_argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          check=False)
    gdb_log.write_bytes(gdb.stdout)
    gdb_text = gdb.stdout.decode(errors="replace")
    lsx_returncode = parse_gdb_guest_status(gdb_text, gdb.returncode)
    lsx_stdout_bytes = lsx_stdout.read_bytes() if lsx_stdout.is_file() else b""
    lsx_stderr_bytes = lsx_stderr.read_bytes() if lsx_stderr.is_file() else b""
    lsx_stdout.write_bytes(lsx_stdout_bytes)
    lsx_stderr.write_bytes(lsx_stderr_bytes)
    (case_dir / "lsx.status").write_text(f"{lsx_returncode}\n")
    readback = re.search(r"<option_enable_lasx>:\s+(-?\d+)$", gdb_text, re.MULTILINE)
    lsx_record = {
        "exit_code": lsx_returncode,
        "signal": signal_for_returncode(lsx_returncode),
        "stdout_bytes": len(lsx_stdout_bytes),
        "stdout_sha256": hashlib.sha256(lsx_stdout_bytes).hexdigest(),
        "stderr_sha256": hashlib.sha256(lsx_stderr_bytes).hexdigest(),
        "option_enable_lasx_readback": int(readback.group(1)) if readback else None,
        "gdb_exit_code": gdb.returncode,
    }

    (case_dir / "replay.sh").write_text(
        "#!/bin/sh\nset -eu\n"
        + "# x86 artifact was captured on xzy86; no remote command is reconstructed here.\n"
        + "env LATX_AVX_CPUID=1 LATX_AOT=0 "
        + shlex.join([str(latx), str(probe)]) + " >lasx.stdout 2>lasx.stderr\n"
        + shlex.join(gdb_argv) + "\n"
    )

    native_record = {
        "exit_code": int(native_status.read_text().strip()),
        "signal": None,
        "stdout_bytes": native_stdout.stat().st_size,
        "stdout_sha256": sha256(native_stdout),
        "binary_sha256": row["native_binary_sha256"],
        "assembly_sha256": row["assembly_sha256"],
        "c_sha256": row["c_sha256"],
    }
    lasx_compare = compare_files(native_stdout, case_dir / "lasx.stdout")
    lsx_compare = compare_files(native_stdout, case_dir / "lsx.stdout")
    result = {
        "mnemonic": mnemonic,
        "probe_sha256": sha256(probe),
        "native": native_record,
        "lasx": lasx_record,
        "lsx": lsx_record,
        "source_sha256": source_digest,
        "latx_binary_sha256": sha256(latx),
        "native_lasx_compare": lasx_compare,
        "native_lsx_compare": lsx_compare,
        "gdb_command": gdb_argv,
    }
    result["classification"] = classify_result(
        native_record, lasx_record, lsx_record, lasx_compare, lsx_compare
    )
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact-root", type=Path, required=True)
    parser.add_argument("--latx", type=Path, required=True)
    parser.add_argument("--expected-latx-sha256", required=True)
    parser.add_argument(
        "--artifact-manifest-sha256", default=EXPECTED_ARTIFACT_MANIFEST_SHA256
    )
    parser.add_argument("--output-root", type=Path, default=Path("/tmp/wi1839-three-way"))
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--execute", action="store_true")
    mode.add_argument("--check-only", action="store_true")
    args = parser.parse_args()

    artifact_root = args.artifact_root.resolve()
    latx = args.latx.resolve()
    rows = validate_artifacts(artifact_root, args.artifact_manifest_sha256)
    actual_latx_sha256 = sha256(latx)
    if actual_latx_sha256 != args.expected_latx_sha256:
        raise SystemExit(
            "LATX binary hash mismatch: "
            f"expected {args.expected_latx_sha256}, got {actual_latx_sha256}"
        )
    source_digest = source_hashes(Path(__file__).resolve().parents[2])

    if not args.execute:
        print("WI-1839 artifact input check complete; guest execution disabled")
        print(f"native_entries={len(rows)}")
        print(f"artifact_manifest_sha256={args.artifact_manifest_sha256}")
        print(f"latx_binary_sha256={actual_latx_sha256}")
        print("lasx_runs=0 lsx_runs=0")
        return 0

    option_addr = option_address(latx)
    args.output_root.mkdir(parents=True, exist_ok=True)
    records = [
        run_one(row, artifact_root, latx, args.output_root.resolve(), source_digest, option_addr)
        for row in rows
    ]
    (args.output_root / "results.json").write_text(
        json.dumps(
            {
                "artifact_manifest_sha256": args.artifact_manifest_sha256,
                "latx_binary_sha256": actual_latx_sha256,
                "source_sha256": source_digest,
                "records": records,
            },
            indent=2,
        )
        + "\n"
    )
    print(f"WROTE WI-1839 three-way records: {args.output_root / 'results.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
