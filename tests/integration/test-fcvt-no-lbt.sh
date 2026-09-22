#!/bin/sh
set -eu
emulator=$(readlink -f "$1")
source_file=$(readlink -f "$2")
smoke_source=$(readlink -f "$3")
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM
if [ -n "${LATX_FCVT_MATRIX_GUEST:-}" ] && [ -n "${LATX_FCVT_SMOKE_GUEST:-}" ]; then
    matrix_guest=$(readlink -f "$LATX_FCVT_MATRIX_GUEST")
    smoke_guest=$(readlink -f "$LATX_FCVT_SMOKE_GUEST")
    continuous_guest=$(readlink -f "${LATX_FCVT_CONTIGUOUS_GUEST:?required with prebuilt guests}")
else
if command -v clang-19 >/dev/null 2>&1; then
    compiler=clang-19
elif command -v clang >/dev/null 2>&1; then
    compiler=clang
else
    echo 'SKIP: clang and lld are required'
    exit 77
fi
"$compiler" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
    "$source_file" -o "$workdir/guest"
"$compiler" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
    "$smoke_source" -o "$workdir/smoke"
"$compiler" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
    -DFCVT_CONTIGUOUS "$source_file" -o "$workdir/continuous"
matrix_guest="$workdir/guest"
smoke_guest="$workdir/smoke"
continuous_guest="$workdir/continuous"
fi
# Native x86-64 reference, including all four x87 rounding modes.
expected=7bb7722a4955dcbfeb321ceafb3a0293c799ebbdaa53341089b05161d0cc72ff
for mode in 0 1 2; do
    for fast in 0 0xffffff; do
        # AOT is enabled by the branch policy. Give each configuration a fresh
        # guest pathname so another mode's cached translation cannot pass it.
        cp "$matrix_guest" "$workdir/matrix-$mode-$fast"
        cp "$smoke_guest" "$workdir/smoke-$mode-$fast"
        cp "$continuous_guest" "$workdir/continuous-$mode-$fast"
        LATX_SOFTFPU=$mode LATX_SOFTFPU_FAST=$fast \
            timeout -k 2 30 "$emulator" -latx-host-hwcap 0x10 \
            "$workdir/smoke-$mode-$fast" > "$workdir/smoke-result"
        LATX_SOFTFPU=$mode LATX_SOFTFPU_FAST=$fast \
            timeout -k 2 30 "$emulator" -latx-host-hwcap 0x10 \
            "$workdir/matrix-$mode-$fast" > "$workdir/result"
        actual=$(sha256sum "$workdir/result")
        test "${actual%% *}" = "$expected" || {
            echo "FAIL: FCVT mode=$mode fast=$fast"
            exit 1
        }
        LATX_SOFTFPU=$mode LATX_SOFTFPU_FAST=$fast \
            timeout -k 2 30 "$emulator" -latx-host-hwcap 0x10 \
            "$workdir/continuous-$mode-$fast" > "$workdir/continuous-result"
        actual=$(sha256sum "$workdir/continuous-result")
        test "${actual%% *}" = d55fbef695982ffbcb984bca32a839f3232b1a345ff70e888310d809ec6648bd || {
            echo "FAIL: continuous FCVT mode=$mode fast=$fast"
            exit 1
        }
    done
done
echo 'PASS: no-LBT x87 conversions and arithmetic match native x86-64'
