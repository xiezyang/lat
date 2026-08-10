#!/usr/bin/env python3
"""Run one AVX fixture against x86, LASX and forced-LSX and emit JSON."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shlex
import struct
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = ROOT / "tests/integration/latx-avx-opt-only-manifest.json"
DEFAULT_LATX = ROOT / "build64/latx-x86_64"
BUILD_SCRIPT = ROOT / "tests/integration/build-latx-avx-single-xzy86.sh"
PRIOR_TSV = ROOT / "docs/avx-validation/t229-three-way-hashes.tsv"
STATE_FIELDS = ("gpr", "xmm", "ymm", "ymm_high", "memory", "mxcsr", "eflags")


def sha256(path: Path) -> str | None:
    if not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def command_text(argv: list[str]) -> str:
    return " ".join(shlex.quote(item) for item in argv)


def run(argv: list[str], *, env: dict[str, str] | None = None) -> subprocess.CompletedProcess:
    merged = os.environ.copy()
    if env:
        merged.update(env)
    return subprocess.run(argv, cwd=ROOT, env=merged, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, check=False)


def ssh_argv(binary: str, config: str | None) -> list[str]:
    argv = [binary]
    if config:
        argv.extend(["-F", config])
    return argv


def read_prior() -> dict[str, list[dict[str, str]]]:
    result: dict[str, list[dict[str, str]]] = {}
    if not PRIOR_TSV.is_file():
        return result
    for line in PRIOR_TSV.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) < 9:
            continue
        row = dict(zip(("mnemonic", "fixture", "x86_sha256", "lasx_sha256",
                        "lsx_sha256", "x86_rc", "lasx_rc", "lsx_rc", "note"),
                       fields[:9]))
        result.setdefault(row["mnemonic"], []).append(row)
    return result


def field_record(status: str = "not_recorded", **values: object) -> dict[str, object]:
    record: dict[str, object] = {"status": status}
    record.update(values)
    return record


def empty_fields() -> dict[str, dict[str, object]]:
    return {name: field_record() for name in STATE_FIELDS}


def parse_trace(stderr: bytes) -> dict[str, dict[str, object]]:
    text = stderr.decode("utf-8", errors="replace")
    samples = []
    for line in text.splitlines():
        if "event=ymm_state" not in line:
            continue
        values = {}
        for token in line.split():
            if "=" in token:
                key, value = token.split("=", 1)
                if key in {"low0", "low1", "shadow_high0", "shadow_high1"}:
                    values[key] = value
        if values:
            samples.append(values)
    fields = empty_fields()
    if samples:
        fields["ymm"] = field_record("recorded", samples=samples)
        fields["ymm_high"] = field_record(
            "recorded",
            samples=[{key: value for key, value in sample.items()
                      if key.startswith("shadow_high")}
                     for sample in samples],
        )
    return fields


def parse_fault(stdout: bytes) -> dict[str, object]:
    result: dict[str, object] = {
        "signal": None,
        "signal_code": None,
        "fault_address": None,
        "fault_address_kind": "not_recorded",
    }
    if len(stdout) >= 16:
        signal, code, offset = struct.unpack_from("<iiQ", stdout)
        if 0 < signal < 128 and code != 0:
            result.update({"signal": signal, "signal_code": code,
                           "fault_address": offset,
                           "fault_address_kind": "fixture_offset"})
    return result


def run_record(label: str, argv: list[str], stdout_path: Path,
               stderr_path: Path, status: int, stdout: bytes,
               stderr: bytes, command: str, fields: dict[str, dict[str, object]] | None = None) -> dict[str, object]:
    stdout_path.write_bytes(stdout)
    stderr_path.write_bytes(stderr)
    fault = parse_fault(stdout)
    observed_signal = fault["signal"]
    if observed_signal is None and status < 0:
        observed_signal = -status
    if observed_signal is None and 128 < status < 192:
        observed_signal = status - 128
    record = {
        "label": label,
        "command": command,
        "argv": argv,
        "exit_status": status,
        "signal": observed_signal,
        "signal_code": fault["signal_code"],
        "fault_address": fault["fault_address"],
        "fault_address_kind": fault["fault_address_kind"],
        "stdout_bytes": len(stdout),
        "stderr_bytes": len(stderr),
        "stdout_sha256": hashlib.sha256(stdout).hexdigest(),
        "stderr_sha256": hashlib.sha256(stderr).hexdigest(),
        "fields": fields or empty_fields(),
    }
    return record


def parse_gdb_status(text: str, fallback: int) -> int:
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
    match = re.search(r"(?:terminated with signal|received signal) (SIG[A-Z0-9]+)", text)
    if match and match.group(1) in signal_numbers:
        return 128 + signal_numbers[match.group(1)]
    return fallback


def option_address(latx: Path) -> str:
    completed = run(["nm", "-g", str(latx)])
    for line in completed.stdout.decode(errors="replace").splitlines():
        words = line.split()
        if len(words) >= 3 and words[2] == "option_enable_lasx":
            return "0x" + words[0]
    raise RuntimeError("option_enable_lasx symbol is missing")


def load_manifest(path: Path) -> dict[str, object]:
    manifest = json.loads(path.read_text())
    summary = manifest.get("summary", {})
    entries = manifest.get("entries", [])
    unique_mnemonics = {entry.get("mnemonic") for entry in entries}
    duplicate_entries = sum(
        max(int(count) - 1, 0)
        for count in summary.get("duplicate_mnemonics", {}).values()
    )
    if summary.get("entry_count") != len(entries) + duplicate_entries:
        raise RuntimeError("manifest entry count does not match its duplicate summary")
    if summary.get("unique_mnemonic_count") != len(unique_mnemonics):
        raise RuntimeError("manifest unique mnemonic count is inconsistent")
    return manifest


def source_hashes(entry: dict[str, object]) -> dict[str, str | None]:
    result = {}
    for relative in entry.get("source_files", []):
        path = ROOT / relative
        result[relative] = sha256(path)
    return result


def base_result(manifest: dict[str, object], entry: dict[str, object],
                latx: Path, prior: dict[str, list[dict[str, str]]]) -> dict[str, object]:
    return {
        "schema_version": 2,
        "manifest_sha256": args_global.manifest_sha256,
        "manifest_source_sha256": manifest["source"]["sha256"],
        "current_source_sha256": args_global.current_source_sha256,
        "current_source_mtime": args_global.current_source_mtime,
        "manifest_input_policy": manifest.get("comparison", {}).get("input_snapshot"),
        "mnemonic": entry["mnemonic"],
        "comparison_key": entry["comparison_key"],
        "source_files": entry["source_files"],
        "source_sha256": source_hashes(entry),
        "translator_functions": entry["translator_functions"],
        "fixture_status": entry["coverage_status"],
        "fixture_runner": entry["runner"],
        "latx_binary": str(latx),
        "latx_binary_sha256": args_global.latx_sha256,
        "x86_is_unique_baseline": True,
        "prior_validation": prior.get(entry["mnemonic"], []),
        "runs": {},
    }


def plan(manifest: dict[str, object], latx: Path, prior: dict[str, list[dict[str, str]]]) -> dict[str, object]:
    entries = []
    for entry in manifest["entries"]:
        result = base_result(manifest, entry, latx, prior)
        if entry["mnemonic"] == "vpsrlq":
            status = "shared_fixture_conflict"
        elif entry["coverage_status"] != "existing_fixture":
            status = "fixture_incomplete"
        else:
            status = "ready_for_execution"
        result.update({"status": status, "outcome": "not_run"})
        entries.append(result)
    counts = {}
    for item in entries:
        counts[item["status"]] = counts.get(item["status"], 0) + 1
    return {"schema_version": 2, "manifest_summary": manifest["summary"],
            "counts": counts, "entries": entries}


def classify(x86: dict[str, object], lasx: dict[str, object],
             lsx: dict[str, object]) -> tuple[str, str]:
    x86_key = (x86["exit_status"], x86["stdout_sha256"])
    lasx_key = (lasx["exit_status"], lasx["stdout_sha256"])
    lsx_key = (lsx["exit_status"], lsx["stdout_sha256"])
    lasx_ok = lasx_key == x86_key
    lsx_ok = lsx_key == x86_key
    if lasx_ok and lsx_ok:
        return "pass_x86_stdout_status", "both backends match the x86 baseline"
    if not lsx_ok and not lasx_ok:
        return "common_error", "LASX and LSX both differ from x86"
    if not lsx_ok:
        return "lsx_error", "LSX differs from the x86 baseline"
    return "lasx_error", "LASX differs from x86 while LSX matches x86"


def execute(args: argparse.Namespace, manifest: dict[str, object],
            entry: dict[str, object], prior: dict[str, list[dict[str, str]]]) -> dict[str, object]:
    latx = Path(args.latx).resolve()
    result = base_result(manifest, entry, latx, prior)
    mnemonic = entry["mnemonic"]
    if mnemonic == "vpsrlq":
        result.update({"status": "shared_fixture_conflict", "outcome": "not_run"})
        return result
    if entry["coverage_status"] != "existing_fixture":
        result.update({"status": "fixture_incomplete", "outcome": "not_run"})
        return result
    if not latx.is_file():
        result.update({"status": "platform_incomplete", "outcome": "not_run",
                       "error": f"missing LATX binary: {latx}"})
        return result

    output_dir = Path(args.output_dir).resolve() / mnemonic
    output_dir.mkdir(parents=True, exist_ok=True)
    remote_dir = f"{args.remote_dir_root.rstrip('/')}/{mnemonic}"
    probe = output_dir / f"latx-avx-single-{mnemonic}.static"
    build_env = {"LATX_SSH_CONFIG": args.ssh_config} if args.ssh_config else {}
    build_script = Path(args_global.build_script).resolve()
    build = run(["bash", str(build_script), mnemonic, args.remote_host,
                 remote_dir, str(probe)], env=build_env)
    result["build"] = {"command": command_text(["bash", str(build_script), mnemonic,
                                                   args.remote_host, remote_dir, str(probe)]),
                        "exit_status": build.returncode,
                        "stdout": build.stdout.decode(errors="replace"),
                        "stderr": build.stderr.decode(errors="replace"),
                        "probe_binary_sha256": sha256(probe)}
    if build.returncode != 0 or not probe.is_file():
        result.update({"status": "platform_incomplete", "outcome": "build_failed"})
        return result

    case_arg = [] if args.case in ("", "default") else [args.case]
    remote_out = f"{remote_dir}/three-way-{args.case or 'default'}.out"
    remote_err = f"{remote_dir}/three-way-{args.case or 'default'}.err"
    remote_status = f"{remote_dir}/three-way-{args.case or 'default'}.status"
    remote_probe = f"{remote_dir}/latx-avx-single-{mnemonic}.static"
    remote_command = "set +e; " + " ".join([shlex.quote(remote_probe)] +
                                             [shlex.quote(item) for item in case_arg])
    remote_command += f" >{shlex.quote(remote_out)} 2>{shlex.quote(remote_err)}; rc=$?; printf '%s\\n' \"$rc\" >{shlex.quote(remote_status)}; exit 0"
    ssh = ssh_argv("ssh", args.ssh_config)
    scp = ssh_argv("scp", args.ssh_config)
    remote = run(ssh + [args.remote_host, remote_command])
    local_x86_out = output_dir / "x86.stdout"
    local_x86_err = output_dir / "x86.stderr"
    local_x86_status = output_dir / "x86.status"
    for remote_file, local_file in ((remote_out, local_x86_out),
                                    (remote_err, local_x86_err),
                                    (remote_status, local_x86_status)):
        copy = run(scp + [f"{args.remote_host}:{remote_file}", str(local_file)])
        if copy.returncode != 0:
            result.update({"status": "platform_incomplete", "outcome": "x86_copy_failed",
                           "error": copy.stderr.decode(errors="replace")})
            return result
    x86_stdout = local_x86_out.read_bytes()
    x86_stderr = local_x86_err.read_bytes()
    x86_status = int(local_x86_status.read_text().strip())
    result["runs"]["x86"] = run_record(
        "x86", [remote_probe] + case_arg, local_x86_out, local_x86_err,
        x86_status, x86_stdout, x86_stderr, f"ssh {args.remote_host} {remote_command}")

    # CPUID=1 keeps the AVX translation path enabled.  LSX is selected only
    # after the GDB run reaches translate_context_init and writes option=0.
    env = {"LATX_AVX_CPUID": "1", "LATX_AOT": "0", "LATX_AVX_TRACE": "3",
           "LATX_AVX_TRACE_YMM": "15", "LATX_AVX_TRACE_YMM_INIT": "1"}
    lasx = run([str(latx), str(probe)] + case_arg, env=env)
    lasx_out = output_dir / "lasx.stdout"
    lasx_err = output_dir / "lasx.stderr"
    lasx_command = ["env"] + [f"{key}={value}" for key, value in sorted(env.items())]
    lasx_command += [str(latx), str(probe)] + case_arg
    result["runs"]["lasx"] = run_record(
        "lasx", [str(latx), str(probe)] + case_arg, lasx_out, lasx_err,
        lasx.returncode, lasx.stdout, lasx.stderr,
        command_text(lasx_command),
        parse_trace(lasx.stderr))

    address = option_address(latx)
    lsx_out = output_dir / "lsx.stdout"
    lsx_err = output_dir / "lsx.stderr"
    gdb_log = output_dir / "lsx.gdb.log"
    run_text = "run " + " ".join([shlex.quote(str(probe))] +
                                  [shlex.quote(item) for item in case_arg])
    gdb_argv = ["gdb", "-q", "-batch", str(latx),
                "-ex", "set pagination off",
                "-ex", "handle SIGSEGV pass nostop noprint",
                "-ex", "handle SIGBUS pass nostop noprint",
                "-ex", "set environment LATX_AVX_CPUID 1",
                "-ex", "set environment LATX_AOT 0",
                "-ex", "set environment LATX_AVX_TRACE 3",
                "-ex", "set environment LATX_AVX_TRACE_YMM 15",
                "-ex", "set environment LATX_AVX_TRACE_YMM_INIT 1",
                "-ex", "break translate_context_init",
                "-ex", f"{run_text} > {shlex.quote(str(lsx_out))} 2> {shlex.quote(str(lsx_err))}",
                "-ex", f"set {{int}}{address} = 0",
                "-ex", f"x/wd {address}", "-ex", "continue"]
    gdb = run(gdb_argv)
    gdb_log.write_bytes(gdb.stdout + gdb.stderr)
    lsx_status = parse_gdb_status(gdb_log.read_text(errors="replace"), gdb.returncode)
    lsx_stdout = lsx_out.read_bytes() if lsx_out.is_file() else b""
    lsx_stderr = lsx_err.read_bytes() if lsx_err.is_file() else b""
    gdb_text = gdb_log.read_text(errors="replace")
    readback_match = re.search(r"<option_enable_lasx>:\s+(-?\d+)$",
                               gdb_text, re.MULTILINE)
    readback_value = int(readback_match.group(1)) if readback_match else None
    readback_confirmed = readback_value == 0
    result["runs"]["lsx"] = run_record(
        "lsx", gdb_argv, lsx_out, lsx_err, lsx_status, lsx_stdout, lsx_stderr,
        command_text(gdb_argv), parse_trace(lsx_stderr))
    result["runs"]["lsx"]["option_enable_lasx_readback"] = readback_value
    result["runs"]["lsx"]["option_enable_lasx_readback_confirmed"] = readback_confirmed
    if not readback_confirmed:
        result.update({"status": "platform_incomplete", "outcome": "lsx_option_unconfirmed"})
        return result
    status, reason = classify(result["runs"]["x86"], result["runs"]["lasx"], result["runs"]["lsx"])
    result.update({"status": status, "outcome": status, "classification_reason": reason})
    return result


def self_test() -> int:
    def record(status: int, digest: str) -> dict[str, object]:
        return {"exit_status": status, "stdout_sha256": digest}
    same = record(0, "same")
    different = record(1, "different")
    cases = [
        ("pass_x86_stdout_status", classify(same, same, same)[0]),
        ("lasx_error", classify(same, different, same)[0]),
        ("lsx_error", classify(same, same, different)[0]),
        ("common_error", classify(same, different, different)[0]),
    ]
    if any(expected != actual for expected, actual in cases):
        print(f"FAIL classifier self-test: {cases}", file=sys.stderr)
        return 1
    print("PASS three-way runner self-test: x86 baseline classifier")
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--build-script", type=Path, default=BUILD_SCRIPT)
    parser.add_argument("--latx", type=Path, default=DEFAULT_LATX)
    parser.add_argument("--remote-host", default="xzy86")
    parser.add_argument("--ssh-config", default=os.environ.get("LATX_SSH_CONFIG"))
    parser.add_argument("--remote-dir-root", default="/tmp/latx-avx-wi1793")
    parser.add_argument("--output-dir", type=Path, default=Path("/tmp/latx-avx-wi1793-results"))
    parser.add_argument("--mnemonic")
    parser.add_argument("--case", default="reference")
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--plan", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    return parser.parse_args()


args_global: argparse.Namespace


def main() -> int:
    global args_global
    args_global = parse_args()
    args_global.manifest_sha256 = sha256(args_global.manifest.resolve())
    args_global.latx_sha256 = sha256(args_global.latx.resolve())
    current_source = ROOT / "target/i386/latx/translator/translate.c"
    args_global.current_source_sha256 = sha256(current_source)
    args_global.current_source_mtime = current_source.stat().st_mtime_ns if current_source.is_file() else None
    if args_global.self_test:
        return self_test()
    manifest = load_manifest(args_global.manifest)
    prior = read_prior()
    if not args_global.execute:
        result = plan(manifest, args_global.latx.resolve(), prior)
        if args_global.mnemonic:
            entry = next((item for item in manifest["entries"]
                          if item["mnemonic"] == args_global.mnemonic.lower()), None)
            if entry is None:
                raise SystemExit(f"unknown mnemonic: {args_global.mnemonic}")
            result = next(item for item in result["entries"]
                          if item["mnemonic"] == args_global.mnemonic.lower())
        print(json.dumps(result, indent=2, sort_keys=False))
        return 0
    if not args_global.mnemonic:
        raise SystemExit("--execute requires --mnemonic; use --plan for the current manifest")
    entry = next((item for item in manifest["entries"]
                  if item["mnemonic"] == args_global.mnemonic.lower()), None)
    if entry is None:
        raise SystemExit(f"unknown mnemonic: {args_global.mnemonic}")
    result = execute(args_global, manifest, entry, prior)
    args_global.output_dir.mkdir(parents=True, exist_ok=True)
    output = args_global.output_dir / f"{entry['mnemonic']}.json"
    output.write_text(json.dumps(result, indent=2, sort_keys=False) + "\n")
    print(json.dumps({"mnemonic": entry["mnemonic"], "status": result["status"],
                      "outcome": result["outcome"], "output": str(output)},
                     sort_keys=True))
    return 0 if result["status"] not in {"platform_incomplete"} else 2


if __name__ == "__main__":
    raise SystemExit(main())
