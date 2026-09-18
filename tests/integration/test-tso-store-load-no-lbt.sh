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

guest="$workdir/latx-tso-store-load-no-lbt"
"$clang" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
    -Wl,--build-id=none "$source_file" -o "$guest"

"$emulator" -latx-host-hwcap 0x10 "$guest"

guest_pc=$(nm "$guest" | awk '$3 == "tso_store_load_site" { print "0x" $1 }')
test -n "$guest_pc"
"$emulator" -latx-host-hwcap 0x10 -latx-show-tb "$guest_pc" "$guest" \
    >"$workdir/tso-store-load-ir2.log" 2>&1

awk '
    /IR2 num =/ { in_ir2 = 1; next }
    /\[LATX\] Assemble IR2/ { in_ir2 = 0 }
    !in_ir2 { next }
    !store && /st\.w/ { store = 1; next }
    store && !load && /dbar/ { exit 1 }
    store && /ld\.w/ { load = 1; next }
    load && /dbar/ { found = 1; exit 0 }
    END { if (!found) exit 1 }
' "$workdir/tso-store-load-ir2.log"

echo "PASS: no-LBT mode omits only the permitted store-to-load barrier"
