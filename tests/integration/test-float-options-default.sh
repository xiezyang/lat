#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
emulator=$(readlink -f "$1")
source_file=$(readlink -f "$2")
arch=$3
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM
if ! command -v nasm >/dev/null 2>&1 || ! command -v ld.lld >/dev/null 2>&1; then
    echo "SKIP: nasm and ld.lld are required"
    exit 77
fi
if [ "$arch" = i386 ]; then
    nasm -DGUEST32=1 -f elf32 "$source_file" -o "$workdir/guest.o"
    ld.lld -m elf_i386 -static "$workdir/guest.o" -o "$workdir/guest"
else
    nasm -f elf64 "$source_file" -o "$workdir/guest.o"
    ld.lld -m elf_x86_64 -static "$workdir/guest.o" -o "$workdir/guest"
fi
for mode in 0 1 2; do
    for fast in 0 -1 0x100000000; do
        LATX_AOT=0 LATX_KZT=0 LATX_MT=0 LATX_SOFTFPU=$mode \
            LATX_SOFTFPU_FAST=$fast LATX_FLOAT80=0 LATX_STACK=0 LATX_128CAL=0 \
            timeout 15 "$emulator" "$workdir/guest"
        echo "PASS: $arch softfpu=$mode fast=$fast new instructions disabled"
    done
done
for mode in 0 1 2; do
    if LATX_AOT=0 LATX_SOFTFPU=$mode LATX_FLOAT80=0 LATX_STACK=1 \
        "$emulator" "$workdir/guest" >"$workdir/invalid.log" 2>&1; then
        echo "FAIL: incompatible hardware stack options were accepted" >&2
        exit 1
    fi
    if ! grep -q 'LATX_STACK requires' "$workdir/invalid.log"; then
        echo "FAIL: missing hardware stack option diagnostic" >&2
        exit 1
    fi
done
echo "PASS: $arch rejects incompatible hardware stack options"
