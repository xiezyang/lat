#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run the x86 correctness fixture through LAT, with explicit feature selection."""
import hashlib
import importlib.util
import os
from pathlib import Path
import platform
import resource
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile


def main():
    emulator, source, arch, mode = sys.argv[1:]
    if platform.system() != 'Linux' or platform.machine() != 'loongarch64':
        print('SKIP: LAT runtime test requires LoongArch64 Linux')
        return 77
    new_hardware = os.environ.get('LATX_TEST_UNALIGNED_ATOMIC') == '1'
    if mode == '1' and not new_hardware:
        print('SKIP: set LATX_TEST_UNALIGNED_ATOMIC=1 on the new hardware')
        return 77
    if mode == '0' and new_hardware:
        print('SKIP: legacy fault-driven mode is tested on old hardware')
        return 77
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    with tempfile.TemporaryDirectory(prefix='latx-atomic-guest-') as directory:
        if mode == '1':
            # An old host could silently pass by interpreting every SIGBUS.
            # Require the raw native instructions and two-CPU test first.
            path = Path(__file__).parents[1] / 'native/test-unaligned-atomic.py'
            spec = importlib.util.spec_from_file_location('native_atomic_probe', path)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            result = module.probe(directory, require=True)
            if result:
                return 1
        supplied = os.environ.get('LATX_ATOMIC_GUEST_DIR')
        if supplied:
            guest = Path(supplied).resolve() / ('unaligned-atomic-' + arch)
            if not guest.is_file() or not os.access(guest, os.X_OK):
                raise RuntimeError('missing executable fixture: ' + str(guest))
        else:
            cc = os.environ.get('LATX_ATOMIC_GUEST_CC')
            if cc:
                command = shlex.split(cc) + (['-m32'] if arch == 'i386' else ['-m64'])
            else:
                clang = shutil.which('clang-19') or shutil.which('clang')
                if not clang or not shutil.which('ld.lld'):
                    print('SKIP: Clang/LLD, LATX_ATOMIC_GUEST_CC or prebuilt fixtures required')
                    return 77
                command = [clang, '--target=' + arch + '-linux-gnu', '-fuse-ld=lld']
            guest = Path(directory) / ('unaligned-atomic-' + arch)
            subprocess.run(command + [
                '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding', '-fno-builtin',
                '-fno-stack-protector', '-fno-pie', '-fomit-frame-pointer',
                '-nostdlib', '-static', '-no-pie', '-Wl,--build-id=none',
            ] + (['-mno-red-zone'] if arch == 'x86_64' else []) + [source, '-o', str(guest)], check=True)
        env = os.environ.copy()
        env.update(LATX_UNALIGNED_ATOMIC=mode, LATX_SOFTFPU='0', LATX_SOFTFPU_FAST='0',
                   LATX_AOT='0', LATX_KZT='0', LATX_CLOSE_PARALLEL='1', LATX_MT='2')
        print('guest sha256=' + hashlib.sha256(guest.read_bytes()).hexdigest(), flush=True)
        print('LATX_UNALIGNED_ATOMIC=' + mode +
              '; softfpu=0; softfpu_fast=0; LATX_MT requested=2 (effective on 16K hosts); AOT=0; KZT=0', flush=True)
        # CLI values are applied after all config files and environment values.
        # These integer-only tests do not change address-translation selection.
        command = [str(Path(emulator).resolve()),
                   '-latx-unaligned-atomic', mode, '-latx-softfpu', '0',
                   '-latx-softfpu-fast', '0', '-latx-close-parallel', '1', str(guest)]
        with subprocess.Popen(command, env=env, cwd=directory, start_new_session=True,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True) as process:
            try:
                output, errors = process.communicate(timeout=120)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.communicate()
                raise
            print(output, end='')
            print(errors, end='', file=sys.stderr)
            if process.returncode or 'PASS: x86 unaligned atomics' not in output:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                raise RuntimeError('guest failed or completion sentinel missing: status=' + str(process.returncode))
        return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.SubprocessError) as error:
        print('FAIL: ' + str(error), file=sys.stderr)
        sys.exit(1)
