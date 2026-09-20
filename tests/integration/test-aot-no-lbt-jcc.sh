#!/bin/sh
set -eu

emulator=$(readlink -f "$1")
source_file=$(readlink -f "$2")
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM

if command -v clang-19 >/dev/null 2>&1; then
    clang=clang-19
elif command -v clang >/dev/null 2>&1; then
    clang=clang
else
    echo "SKIP: clang is required to build the x86_64 guest"
    exit 77
fi

guest="$workdir/aot-no-lbt-jcc"
cache_home="$workdir/home"
mkdir -p "$cache_home"

"$clang" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
    -Wl,--build-id=none "$source_file" -o "$guest"

env -u LATX_SOFTFPU HOME="$cache_home" LATX_AOT=1 \
    timeout -k 2s 30s "$emulator" -latx-host-hwcap 0x10 "$guest"

for _ in $(seq 1 30); do
    if find "$cache_home/.cache/latx" -type f -name 'v2-*.aot2' \
        -size +0c -print -quit 2>/dev/null | grep -q .; then
        break
    fi
    sleep 1
done

find "$cache_home/.cache/latx" -type f -name 'v2-*.aot2' -size +0c \
    -print -quit 2>/dev/null | grep -q .

env -u LATX_SOFTFPU HOME="$cache_home" LATX_AOT=1 LATX_AOT_SCAN=1 \
    timeout -k 2s 30s "$emulator" -latx-host-hwcap 0x10 "$guest" \
    >"$workdir/load.out" 2>"$workdir/load.err"

grep -q 'AOT_SCAN_LOAD_SUCCESS' "$workdir/load.err"
echo "PASS: no-LBT AOT loads a cached Jcc loop"
