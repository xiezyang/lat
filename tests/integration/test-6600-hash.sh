#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
emulator=$(readlink -f "$1")
source_file=$(readlink -f "$2")
abi=$3
case "$abi" in
    i386) format=elf32; machine=elf_i386 ;;
    x86_64) format=elf64; machine=elf_x86_64 ;;
    *) echo "unsupported ABI: $abi" >&2; exit 1 ;;
esac
if ! command -v nasm >/dev/null 2>&1 || ! command -v ld.lld >/dev/null 2>&1; then
    echo 'SKIP: nasm and ld.lld are required'
    exit 77
fi
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM
nasm -f "$format" "$source_file" -o "$workdir/guest.o"
ld.lld -m "$machine" -static "$workdir/guest.o" -o "$workdir/guest"
python3 - "$workdir/guest" <<'PY'
import subprocess
import sys
symbols = {}
for line in subprocess.check_output(["nm", "-n", sys.argv[1]], text=True).splitlines():
    fields = line.split()
    if len(fields) == 3 and fields[2] in ("target_a", "target_b"):
        symbols[fields[2]] = int(fields[0], 16)
def index(pc):
    return sum((((pc >> bit) ^ (pc >> (bit + 16))) & 1) << bit
               for bit in range(16))
assert len(symbols) == 2, symbols
assert symbols["target_a"] ^ symbols["target_b"] == 0x10001, symbols
assert index(symbols["target_a"]) == index(symbols["target_b"]), symbols
assert index(symbols["target_a"]) > 4095, symbols
print("PASS: independently checked 16-bit HASH collision addresses", symbols)
PY
modes=0
case ${LATX_TEST_6600_HASH:-0} in
    0) ;;
    1) modes='0 1' ;;
    *) echo 'LATX_TEST_6600_HASH must be 0 or 1' >&2; exit 1 ;;
esac
for mode in $modes; do
    LATX_HASH=$mode LATX_AOT=0 LATX_TU=0 LATX_KZT=0 LATX_MT=0 \
        LATX_SOFTFPU=0 LATX_SOFTFPU_FAST=0 LATX_FLOAT80=0 LATX_STACK=0 \
        LATX_128CAL=0 timeout 30 "$emulator" "$workdir/guest"
    echo "PASS: $abi indirect HASH collision loop, hash=$mode softfpu=0 fast=0"
done
