#!/bin/sh
set -eu

emulator=$(readlink -f "$1")
source_file=$(readlink -f "$2")
indirect_source=$(readlink -f "$3")
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM

if command -v clang-19 >/dev/null 2>&1 && command -v ld.lld >/dev/null 2>&1; then
    compiler=clang-19
elif command -v clang >/dev/null 2>&1 && command -v ld.lld >/dev/null 2>&1; then
    compiler=clang
elif [ "$(uname -m)" = x86_64 ] && command -v gcc >/dev/null 2>&1; then
    compiler=gcc
else
    echo "SKIP: x86_64 assembler/linker is required to build the guest"
    exit 77
fi

if [ "$compiler" = gcc ]; then
    "$compiler" -nostdlib -static -no-pie -Wl,--build-id=none \
        "$source_file" -o "$workdir/no-lbt-ret-eflags"
    "$compiler" -nostdlib -static -no-pie -Wl,--build-id=none \
        "$indirect_source" -o "$workdir/no-lbt-ret-indirect-caller"
else
    "$compiler" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
        -Wl,--build-id=none "$source_file" -o "$workdir/no-lbt-ret-eflags"
    "$compiler" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
        -Wl,--build-id=none "$indirect_source" -o "$workdir/no-lbt-ret-indirect-caller"
fi

if [ "$(uname -m)" = x86_64 ]; then
    "$workdir/no-lbt-ret-eflags"
    "$workdir/no-lbt-ret-indirect-caller"
fi
for mode in 0 1; do
    mkdir -p "$workdir/cache-$mode"
    for phase in cold warm; do
        env HOME="$workdir/cache-$mode" LATX_AOT=1 LATX_TU_RET_EFLAGS="$mode" \
            "$emulator" -latx-host-hwcap 0x10 "$workdir/no-lbt-ret-eflags"
        env HOME="$workdir/cache-$mode" LATX_AOT=1 LATX_TU_RET_EFLAGS="$mode" \
            "$emulator" -latx-host-hwcap 0x10 "$workdir/no-lbt-ret-indirect-caller"
    done
done
echo "PASS: no-LBT preserves EFLAGS across direct/indirect callers and RET-load SIGSEGV (0/1, cold/warm)"
