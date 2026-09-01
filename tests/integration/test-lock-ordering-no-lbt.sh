#!/bin/sh
set -eu

emulator=$(readlink -f "$1")
xadd_source=$(readlink -f "$2")
ordering_source=$(readlink -f "$3")
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM

if command -v clang-19 >/dev/null 2>&1; then
    clang=clang-19
elif command -v clang >/dev/null 2>&1; then
    clang=clang
else
    echo "SKIP: clang is required to build the x86_64 guests"
    exit 77
fi

for source in "$xadd_source" "$ordering_source"; do
    output="$workdir/$(basename "$source" .S)"
    "$clang" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
        -Wl,--build-id=none "$source" -o "$output"
    "$emulator" -latx-host-hwcap 0x10 "$output"
done

xadd_guest="$workdir/latx-lock-xadd-stress"
xadd_pc=$(nm "$xadd_guest" | awk '$3 == "lock_xadd_site" { print "0x" $1 }')
test -n "$xadd_pc"

"$emulator" -latx-host-hwcap 0x10 -latx-show-tb "$xadd_pc" "$xadd_guest" \
    >"$workdir/xadd-ir2.log" 2>&1

awk '
    /IR2 num =/ { in_ir2 = 1; next }
    /\[LATX\] Assemble IR2/ { in_ir2 = 0 }
    in_ir2 && /sc\.w/ { after_sc = 1; next }
    after_sc && /dbar/ { found = 1; exit 0 }
    after_sc && /ld\.w/ { exit 1 }
    END { if (!found) exit 1 }
' "$workdir/xadd-ir2.log"

echo "PASS: no-LBT locked operations preserve atomicity and x86 ordering"
