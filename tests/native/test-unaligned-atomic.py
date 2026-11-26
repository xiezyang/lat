#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build/run the native capability probe without a translator."""
import hashlib
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import sys
import tempfile


def probe(directory, require=False):
    source = Path(__file__).with_name('loongarch-unaligned-atomic.c')
    supplied = os.environ.get('LATX_ATOMIC_NATIVE_PROBE')
    if supplied:
        binary = Path(supplied).resolve()
        if not binary.is_file() or not os.access(binary, os.X_OK):
            raise RuntimeError('LATX_ATOMIC_NATIVE_PROBE is not executable')
    else:
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            print('SKIP: a native C compiler or LATX_ATOMIC_NATIVE_PROBE is required')
            return 77
        binary = Path(directory) / 'native-unaligned-atomic'
        subprocess.run(cc + ['-O2', '-Wall', '-Wextra', '-Werror', '-pthread',
                             str(source), '-o', str(binary)], check=True)
    print('native probe sha256=' + hashlib.sha256(binary.read_bytes()).hexdigest(), flush=True)
    r = subprocess.run([str(binary)] + (['--require'] if require else []), timeout=90)
    if require and r.returncode == 77:
        raise RuntimeError('requested hardware support was not verified')
    return r.returncode if r.returncode >= 0 else 1


def main():
    if platform.system() != 'Linux' or platform.machine() != 'loongarch64':
        print('SKIP: native probe requires LoongArch64 Linux')
        return 77
    with tempfile.TemporaryDirectory(prefix='latx-atomic-native-') as directory:
        return probe(directory, os.environ.get('LATX_TEST_UNALIGNED_ATOMIC') == '1')


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.SubprocessError) as error:
        print('FAIL: ' + str(error), file=sys.stderr)
        sys.exit(1)
