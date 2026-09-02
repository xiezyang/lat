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

guest="$workdir/latx-tso-ordering-no-lbt"
"$clang" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
    -Wl,--build-id=none "$source_file" -o "$guest"

"$emulator" -latx-host-hwcap 0x10 "$guest"

guest_pc=$(nm "$guest" | awk '$3 == "tso_store_site" { print "0x" $1 }')
test -n "$guest_pc"
"$emulator" -latx-host-hwcap 0x10 -latx-show-tb "$guest_pc" "$guest" \
    >"$workdir/tso-ir2.log" 2>&1

awk '
    /IR2 num =/ { in_ir2 = 1; next }
    /\[LATX\] Assemble IR2/ { in_ir2 = 0 }
    in_ir2 && /st\.w/ { after_store = 1; next }
    after_store && /dbar/ { found = 1; exit 0 }
    after_store && /st\.w/ { exit 1 }
    END { if (!found) exit 1 }
' "$workdir/tso-ir2.log"

echo "PASS: no-LBT mode preserves x86 TSO ordering"
