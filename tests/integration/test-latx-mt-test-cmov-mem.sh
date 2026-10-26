#!/bin/sh
set -eu

emulator=$(readlink -f "$1")
source_file=$(readlink -f "$2")
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM

if [ "$(getconf PAGESIZE)" != 16384 ]; then
    echo "SKIP: LATX_MT requires a 16K host page"
    exit 77
fi

if ! command -v nasm >/dev/null 2>&1; then
    echo "SKIP: nasm is required to build the x86_64 guest"
    exit 77
fi

if ! command -v ld.lld >/dev/null 2>&1; then
    echo "SKIP: ld.lld is required to build the x86_64 guest"
    exit 77
fi

guest="$workdir/latx-mt-test-cmov-mem"
nasm -f elf64 "$source_file" -o "$workdir/guest.o"
ld.lld -m elf_x86_64 -static --build-id=none "$workdir/guest.o" -o "$guest"

for mode in 0 1 2; do
    if ! LATX_AOT=0 LATX_KZT=0 LATX_MT="$mode" \
        timeout 30 "$emulator" "$guest"; then
        echo "FAIL: LATX_MT=$mode did not execute TEST+CMOVcc" >&2
        exit 1
    fi
done

echo "PASS: TEST+CMOVcc memory pair runs with LATX_MT=0/1/2"
