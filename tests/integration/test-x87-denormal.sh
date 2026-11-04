#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
emulator=$(readlink -f "$1")
source_file=$(readlink -f "$2")
arch=$3
unmasked_source=$(dirname "$source_file")/x87-denormal-unmasked.asm
ulimit -c 0
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM
if ! command -v nasm >/dev/null 2>&1 || ! command -v ld.lld >/dev/null 2>&1; then
    echo 'SKIP: nasm and ld.lld are required'
    exit 77
fi
case "$arch" in
    i386) format=elf32; emulation=elf_i386 ;;
    x86_64) format=elf64; emulation=elf_x86_64 ;;
    *) echo "Unsupported architecture: $arch" >&2; exit 2 ;;
esac
nasm -f "$format" "$source_file" -o "$workdir/guest.o"
ld.lld -m "$emulation" -static "$workdir/guest.o" -o "$workdir/guest"
float80=${LATX_TEST_FLOAT80:-0}
stack=${LATX_TEST_STACK:-0}
fast_modes=0
if [ "$float80" != 0 ]; then
    fast_modes='0 0x10000 0x1fffffffff'
fi
for fast in $fast_modes; do
    LATX_AOT=0 LATX_KZT=0 LATX_MT=0 LATX_SOFTFPU=2 \
        LATX_SOFTFPU_FAST=$fast LATX_FLOAT80=$float80 LATX_STACK=$stack \
        LATX_128CAL=0 timeout 30 "$emulator" "$workdir/guest"
    echo "PASS: $arch denormal operand flags float80=$float80 stack=$stack fast=$fast"
done

# These cases must retain the source and TOP, then report pending D at FWAIT.
for case_id in 1 2 3 4 5 6; do
    nasm -DTEST_CASE=$case_id -f "$format" "$unmasked_source" -o "$workdir/unmasked.o"
    ld.lld -m "$emulation" -static "$workdir/unmasked.o" -o "$workdir/unmasked"
    case "$case_id" in 1|6) expected=82b8 ;; *) expected=82b0 ;; esac
    for fast in $fast_modes; do
        status=0
        LATX_AOT=0 LATX_KZT=0 LATX_MT=0 LATX_SOFTFPU=2 \
            LATX_SOFTFPU_FAST=$fast LATX_FLOAT80=$float80 LATX_STACK=$stack \
            LATX_128CAL=0 timeout 30 "$emulator" "$workdir/unmasked" \
            >"$workdir/status" 2>"$workdir/stderr" || status=$?
        actual=$(od -An -tx1 -v "$workdir/status" | tr -d ' \n')
        if [ "$status" != 136 ] || [ "$actual" != "$expected" ]; then
            echo "FAIL: $arch unmasked D case=$case_id fast=$fast exit=$status status=$actual" >&2
            cat "$workdir/stderr" >&2
            exit 1
        fi
    done
    echo "PASS: $arch unmasked D case=$case_id preserves source/TOP and reports SIGFPE"
done
