#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
emulator=$(readlink -f "$1")
source_file=$(readlink -f "$2")
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM
if ! command -v nasm >/dev/null 2>&1 || ! command -v ld.lld >/dev/null 2>&1; then
    echo 'SKIP: nasm and ld.lld are required'
    exit 77
fi
float80=${LATX_TEST_FLOAT80:-0}
stack=${LATX_TEST_STACK:-0}
if [ "$float80" != 0 ]; then
    nasm -DCHECK_NATIVE_FLAGS=1 -f elf32 "$source_file" -o "$workdir/guest.o"
else
    nasm -f elf32 "$source_file" -o "$workdir/guest.o"
fi
ld.lld -m elf_i386 -static "$workdir/guest.o" -o "$workdir/guest"
for fast in 0 0x600000; do
    LATX_AOT=0 LATX_KZT=0 LATX_MT=0 LATX_SOFTFPU=2 \
        LATX_SOFTFPU_FAST=$fast LATX_FLOAT80=$float80 LATX_STACK=$stack LATX_128CAL=0 \
        timeout 30 "$emulator" "$workdir/guest"
    echo "PASS: i386 FIST/FISTP signed 16-bit range float80=$float80 stack=$stack fast=$fast"
done
