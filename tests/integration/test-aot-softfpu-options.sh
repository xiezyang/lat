#!/bin/sh
# Pass a static x86-64 guest that exits successfully (for example a flag test).
set -eu
emulator=$(readlink -f "$1")
guest=$(readlink -f "$2")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM

for mode in implicit explicit order-a order-b disabled; do
    cache="$work/$mode"
    mkdir -p "$cache"
    set --
    case "$mode" in
        implicit) set -- -latx-aot 1 ;;
        explicit) set -- -latx-softfpu 2 -latx-aot 1 ;;
        order-a) set -- -latx-aot 1 -latx-softfpu 2 ;;
        order-b) set -- -latx-softfpu 2 -latx-aot 1 ;;
        disabled) set -- -latx-aot 0 -latx-softfpu 2 ;;
    esac
    if [ "$mode" = explicit ]; then
        env -u LATX_SOFTFPU HOME="$cache" LATX_AOT=1 \
            timeout 30 "$emulator" "$guest" > "$work/$mode.out"
    else
        env -u LATX_AOT -u LATX_SOFTFPU HOME="$cache" \
            timeout 30 "$emulator" "$@" "$guest" > "$work/$mode.out"
    fi
    if [ "$mode" = disabled ]; then
        test -z "$(find "$cache" -name '*.aot2' -size +0c -print -quit)"
        continue
    fi
    count=0
    while [ -z "$(find "$cache" -name '*.aot2' -size +0c -print -quit)" ]; do
        count=$((count + 1))
        test "$count" -lt 30
        sleep 1
    done
done
echo 'PASS: explicit AOT and softfpu option ordering preserve cache generation'
