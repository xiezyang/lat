#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LAT Project Authors
# SPDX-License-Identifier: GPL-2.0-only
"""Run self-checking AVX guests in JIT and isolated cold/hot AOT paths."""
import argparse
import hashlib
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
import time


def run_case(emulator, source, work, prebuilt):
    guest = work / source.stem
    if prebuilt:
        shutil.copy2(prebuilt / source.stem, guest)
    else:
        if platform.machine() == "x86_64" and shutil.which("gcc"):
            compiler = ["gcc"]
        else:
            clang = shutil.which("clang-19") or shutil.which("clang")
            compiler = [clang, "--target=x86_64-linux-gnu", "-fuse-ld=lld"]
        subprocess.run(compiler + ["-nostdlib", "-static", "-Wl,--build-id=none",
                                  str(source), "-o", str(guest)], check=True)
    if emulator == "native":
        subprocess.run([str(guest)], check=True, timeout=30)
        return

    # The executable's unique absolute path is part of the AOT cache key.
    # Never reuse another test's cache or change the user's HOME/configuration.
    cache = Path.home() / ".cache" / "latx"
    key = hashlib.sha256(str(guest).encode()).hexdigest()
    pattern = "v2-*-" + key + ".aot2"
    env = os.environ.copy()
    try:
        for mode in ("jit", "cold", "hot"):
            env["LATX_AOT"] = "0" if mode == "jit" else "1"
            subprocess.run([emulator, str(guest)], env=env,
                           check=True, timeout=30)
            if mode == "cold":
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    if any(p.stat().st_size for p in cache.glob(pattern)):
                        break
                    time.sleep(0.1)
                else:
                    raise RuntimeError("no non-empty AOT cache for " + source.name)
    finally:
        # Only remove cache artifacts keyed by this temporary executable.
        for artifact in cache.glob(pattern + "*"):
            artifact.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prebuilt", type=Path,
                        default=os.environ.get("LATX_AVX_PREBUILT"),
                        help="guests compiled on x86 (or LATX_AVX_PREBUILT)")
    parser.add_argument("emulator", help="LAT executable, or native")
    parser.add_argument("sources", type=Path, nargs="+")
    args = parser.parse_args()
    if not args.prebuilt and not (
            (platform.machine() == "x86_64" and shutil.which("gcc")) or
            shutil.which("clang-19") or shutil.which("clang")):
        print("SKIP: an x86 compiler or --prebuilt guests are required")
        return 77
    emulator = args.emulator
    if emulator != "native":
        emulator = str(Path(emulator).resolve())
    failures = 0
    with tempfile.TemporaryDirectory(prefix="latx-avx-regression-") as directory:
        for source in args.sources:
            try:
                run_case(emulator, source.resolve(), Path(directory), args.prebuilt)
                print("PASS:", source.name, flush=True)
            except (subprocess.SubprocessError, RuntimeError) as error:
                failures += 1
                print("FAIL:", source.name, error, flush=True)
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
