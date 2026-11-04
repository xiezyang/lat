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
nasm -f elf32 "$source_file" -o "$workdir/guest.o"
ld.lld -m elf_i386 -static "$workdir/guest.o" -o "$workdir/guest"
float80=${LATX_TEST_FLOAT80:-0}
stack=${LATX_TEST_STACK:-0}
fast_modes=0
if [ "$float80" != 0 ]; then
    fast_modes='0 0xc000'
fi
for fast in $fast_modes; do
    LATX_AOT=0 LATX_KZT=0 LATX_MT=0 LATX_SOFTFPU=2 \
        LATX_SOFTFPU_FAST=$fast LATX_FLOAT80=$float80 LATX_STACK=$stack \
        LATX_128CAL=0 timeout 30 "$emulator" "$workdir/guest"
    echo "PASS: i386 extended FRNDINT/FSCALE float80=$float80 stack=$stack fast=$fast"
done
