#!/usr/bin/env python3
"""Audit CONFIG_LATX_AVX_OPT entries and generate honest test entry points."""

import argparse
import hashlib
import json
import stat
import sys
from pathlib import Path
import re


ENTRY_RE = re.compile(
    r"\bTRANS_FUNC_GEN\(\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*\)"
)
TRUE_RE = re.compile(r"^\s*#define\s+LATX_AVX_OPT_ENTRY\s+true\s*$")
FALSE_RE = re.compile(r"^\s*#define\s+LATX_AVX_OPT_ENTRY\s+false\s*$")

KNOWN_OPERANDS = {
    "vinserti128": ["ymm, ymm, xmm, imm8", "ymm, ymm, m128, imm8"],
    "vpsrlq": [
        "xmm/ymm, xmm/ymm, imm8",
        "xmm/ymm, xmm/ymm, xmm/m128",
    ],
    "vpbroadcastq": ["xmm/ymm, m64", "xmm/ymm, xmm"],
}

CATEGORY_BOUNDARIES = {
    "compare": ["ordered and unordered values", "NaN", "signed zero"],
    "conversion": ["zero", "signed minimum", "signed maximum", "overflow"],
    "shift": ["count 0", "count width-1", "count width", "count 255"],
    "rounding": ["imm8 0", "imm8 1", "imm8 4", "imm8 8", "imm8 255"],
    "gather": ["base only", "index zero", "negative index", "page boundary"],
    "move": ["register alias", "unaligned memory", "page boundary"],
    "broadcast": ["zero", "all ones", "scalar at page end"],
    "insert_extract": ["imm8 0", "imm8 1", "imm8 width-1", "imm8 255"],
    "arithmetic": ["zero", "all ones", "signed minimum", "signed maximum"],
    "fma": ["register alias", "memory source", "positive/negative zero",
            "infinity", "quiet/signaling NaN", "subnormal", "rounding", "MXCSR exceptions"],
}


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def default_source(root: Path) -> Path:
    return root / "target/i386/latx/translator/translate.c"


def category_for(mnemonic: str) -> str:
    if mnemonic.startswith(("VCMP", "VCOMI", "VUCOMI")):
        return "compare"
    if mnemonic.startswith("VCVT"):
        return "conversion"
    if mnemonic.startswith(("VPSR", "VPSL")):
        return "shift"
    if mnemonic.startswith("VROUND"):
        return "rounding"
    if "GATHER" in mnemonic:
        return "gather"
    if mnemonic.startswith(("VINSERT", "VEXTRACT")):
        return "insert_extract"
    if "BROADCAST" in mnemonic:
        return "broadcast"
    if mnemonic.startswith(("VMOV", "VLD", "VST")):
        return "move"
    if mnemonic.startswith(("VFM", "VFN")):
        return "fma"
    if mnemonic.startswith("VAES"):
        return "aes"
    if mnemonic.startswith(("VPADD", "VPSUB", "VPMUL", "VPMIN", "VPMAX")):
        return "arithmetic"
    if mnemonic.startswith("VP"):
        return "packed_integer"
    return "other"


def boundary_inputs(category: str) -> list[str]:
    return CATEGORY_BOUNDARIES.get(category, ["requires_manual_template"])


def parse_entries(source: Path) -> list[dict]:
    entries = []
    avx_opt_only = False
    for line_number, raw_line in enumerate(source.read_text().splitlines(), 1):
        if TRUE_RE.match(raw_line):
            avx_opt_only = True
            continue
        if FALSE_RE.match(raw_line):
            avx_opt_only = False
            continue
        if not avx_opt_only or raw_line.lstrip().startswith("//"):
            continue
        match = ENTRY_RE.search(raw_line)
        if match:
            opcode, translator = match.groups()
            entries.append(
                {
                    "source_line": line_number,
                    "opcode": opcode,
                    "mnemonic": opcode.lower(),
                    "translator_function": translator,
                }
            )
    return entries


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def fixture_info(root: Path, mnemonic: str) -> dict:
    integration = root / "tests/integration"
    stem = f"latx-avx-single-{mnemonic}"
    paths = [integration / f"{stem}.S", integration / f"{stem}.c"]
    runner = integration / f"test-{stem}.sh"
    present = all(path.is_file() for path in paths) and runner.is_file()
    return {
        "status": "existing_fixture" if present else "needs_manual_template",
        "source_files": [str(path.relative_to(root)) for path in paths if path.is_file()],
        "runner": str(runner.relative_to(root)) if runner.is_file() else None,
    }


def build_manifest(root: Path, source: Path) -> dict:
    occurrences = parse_entries(source)
    grouped = {}
    for occurrence in occurrences:
        grouped.setdefault(occurrence["mnemonic"], []).append(occurrence)

    entries = []
    for mnemonic in sorted(grouped):
        source_entries = grouped[mnemonic]
        category = category_for(mnemonic.upper())
        fixture = fixture_info(root, mnemonic)
        entries.append(
            {
                "mnemonic": mnemonic,
                "category": category,
                "occurrences": source_entries,
                "translator_functions": sorted(
                    {item["translator_function"] for item in source_entries}
                ),
                "operand_forms": KNOWN_OPERANDS.get(mnemonic, []),
                "operand_forms_status": (
                    "catalogued" if mnemonic in KNOWN_OPERANDS else "needs_manual_catalog"
                ),
                "boundary_inputs": boundary_inputs(category),
                "manual_template_required": fixture["status"] != "existing_fixture",
                "coverage_status": fixture["status"],
                "source_files": fixture["source_files"],
                "runner": fixture["runner"],
                "generation_entry": (
                    "tests/integration/generate-latx-avx-single-mnemonic.py"
                    f" generate --mnemonic {mnemonic} --output-dir <dir>"
                ),
                "comparison_key": mnemonic,
            }
        )

    duplicate_mnemonics = {
        mnemonic: len(items)
        for mnemonic, items in grouped.items()
        if len(items) > 1
    }
    return {
        "schema_version": 1,
        "source": {
            "path": str(source.relative_to(root)),
            "sha256": sha256(source),
        },
        "summary": {
            "entry_count": len(occurrences),
            "unique_mnemonic_count": len(entries),
            "duplicate_mnemonics": dict(sorted(duplicate_mnemonics.items())),
        },
        "generation": {
            "generator": "tests/integration/generate-latx-avx-single-mnemonic.py",
            "static_check": "tests/integration/check-latx-avx-single-mnemonic.sh",
            "manual_template_exit_code": 77,
            "manual_template_status": "NEEDS_MANUAL_TEMPLATE",
        },
        "comparison": {
            "consumer": "WI-1793 LASX/LSX three-way runner",
            "runner": "tests/integration/run-latx-avx-three-way.py",
            "status_values": [
                "pass_x86_stdout_status",
                "lasx_error",
                "lsx_error",
                "common_error",
                "fixture_incomplete",
                "baseline_incomplete",
                "shared_fixture_conflict",
                "platform_incomplete",
            ],
            "artifacts": [
                "native/reference.bin",
                "latx/result.bin",
                "lsx/result.bin",
                "lasx/result.bin",
            ],
            "required_fields": [
                "source_sha256",
                "binary_sha256",
                "mnemonic_set",
                "native_exit_status",
                "native_stdout_sha256",
                "latx_stdout_sha256",
                "lsx_stdout_sha256",
                "lasx_stdout_sha256",
                "fixture_status",
                "fixture_runner",
                "source_files",
                "source_sha256",
                "latx_binary_sha256",
                "runs.x86.exit_status",
                "runs.x86.signal",
                "runs.x86.signal_code",
                "runs.x86.fault_address",
                "runs.x86.fields.gpr",
                "runs.x86.fields.xmm",
                "runs.x86.fields.ymm",
                "runs.x86.fields.ymm_high",
                "runs.x86.fields.memory",
                "runs.x86.fields.mxcsr",
                "runs.x86.fields.eflags",
                "runs.lasx.exit_status",
                "runs.lasx.signal",
                "runs.lasx.signal_code",
                "runs.lasx.fault_address",
                "runs.lasx.fields.gpr",
                "runs.lasx.fields.xmm",
                "runs.lasx.fields.ymm",
                "runs.lasx.fields.ymm_high",
                "runs.lasx.fields.memory",
                "runs.lasx.fields.mxcsr",
                "runs.lasx.fields.eflags",
                "runs.lsx.exit_status",
                "runs.lsx.signal",
                "runs.lsx.signal_code",
                "runs.lsx.fault_address",
                "runs.lsx.fields.gpr",
                "runs.lsx.fields.xmm",
                "runs.lsx.fields.ymm",
                "runs.lsx.fields.ymm_high",
                "runs.lsx.fields.memory",
                "runs.lsx.fields.mxcsr",
                "runs.lsx.fields.eflags",
            ],
        },
        "entries": entries,
    }


def json_text(value: dict) -> str:
    return json.dumps(value, indent=2, sort_keys=False) + "\n"


def check_manifest(root: Path, manifest_path: Path, source: Path) -> None:
    actual = build_manifest(root, source)
    expected = json.loads(manifest_path.read_text())
    if actual != expected:
        print(f"FAIL manifest is stale: {manifest_path}", file=sys.stderr)
        print("regenerate with: manifest --output " + str(manifest_path), file=sys.stderr)
        raise SystemExit(1)
    print(
        "PASS manifest current: "
        f"entries={actual['summary']['entry_count']} "
        f"unique={actual['summary']['unique_mnemonic_count']}"
    )


def write_executable(path: Path, content: str) -> None:
    path.write_text(content)
    path.chmod(path.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def generate_entry(root: Path, source: Path, mnemonic: str, output_dir: Path) -> None:
    manifest = build_manifest(root, source)
    matches = [entry for entry in manifest["entries"] if entry["mnemonic"] == mnemonic]
    if not matches:
        raise SystemExit(f"unknown AVX mnemonic: {mnemonic}")
    entry = matches[0]
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / f"{mnemonic}.json").write_text(json_text(entry))
    wrapper = output_dir / f"test-latx-avx-single-{mnemonic}.sh"
    if entry["coverage_status"] == "existing_fixture":
        runner = root / entry["runner"]
        write_executable(
            wrapper,
            "#!/bin/sh\nset -eu\n"
            f'exec "{runner}" "$@"\n',
        )
        print(f"PASS generated existing entry: {mnemonic} -> {runner}")
    else:
        write_executable(
            wrapper,
            "#!/bin/sh\nset -eu\n"
            f'echo "NEEDS_MANUAL_TEMPLATE mnemonic={mnemonic}" >&2\n'
            "exit 77\n",
        )
        print(f"NEEDS_MANUAL_TEMPLATE generated entry: {mnemonic}")


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source", type=Path, default=default_source(root), help="translate.c path"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    manifest_parser = subparsers.add_parser("manifest")
    manifest_parser.add_argument("--output", type=Path, required=True)

    check_parser = subparsers.add_parser("check")
    check_parser.add_argument("--manifest", type=Path, required=True)

    subparsers.add_parser("summary")
    list_parser = subparsers.add_parser("list")
    list_parser.add_argument(
        "--status", choices=("all", "existing", "manual"), default="all"
    )

    generate_parser = subparsers.add_parser("generate")
    generate_parser.add_argument("--mnemonic", required=True)
    generate_parser.add_argument("--output-dir", type=Path, required=True)

    args = parser.parse_args()
    source = args.source.resolve()
    if not source.is_file():
        parser.error(f"source does not exist: {source}")

    if args.command == "manifest":
        manifest = build_manifest(root, source)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json_text(manifest))
        print(
            f"PASS generated manifest: entries={manifest['summary']['entry_count']} "
            f"unique={manifest['summary']['unique_mnemonic_count']} "
            f"output={args.output}"
        )
    elif args.command == "check":
        check_manifest(root, args.manifest, source)
    elif args.command == "summary":
        manifest = build_manifest(root, source)
        print(json.dumps(manifest["summary"], sort_keys=True))
    elif args.command == "list":
        manifest = build_manifest(root, source)
        for entry in manifest["entries"]:
            if args.status == "existing" and entry["coverage_status"] != "existing_fixture":
                continue
            if args.status == "manual" and entry["coverage_status"] != "needs_manual_template":
                continue
            print(entry["mnemonic"])
    elif args.command == "generate":
        generate_entry(root, source, args.mnemonic.lower(), args.output_dir.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
