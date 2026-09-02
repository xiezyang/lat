#!/bin/sh
set -eu

latx=$(readlink -f "$1")
reference=$(readlink -f "$2")
test_dir=$(dirname "$(readlink -f "$0")")
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

build_guest()
{
    source=$1
    output=$2
    shift 2
    "$clang" --target=x86_64-linux-gnu -fuse-ld=lld -nostdlib -static \
        -Wl,--build-id=none "$@" "$source" -o "$output"
}

build_guest "$test_dir/latx-eflags-branch-differential.S" \
    "$workdir/eflags-branch"
build_guest "$test_dir/latx-eflags-differential.S" \
    "$workdir/eflags-random" -DTRACE_RECORD=1

"$reference" -d exec,cpu,nochain -D "$workdir/branch.qemu-tb.log" \
    "$workdir/eflags-branch" >"$workdir/branch.reference"
"$latx" -latx-host-hwcap 0x10 -latx-tb-state 10 \
    "$workdir/eflags-branch" >"$workdir/branch.latx" \
    2>"$workdir/branch.latx-tb.log"
cmp "$workdir/branch.reference" "$workdir/branch.latx"
python3 "$test_dir/compare-latx-tb-state.py" \
    "$workdir/branch.qemu-tb.log" "$workdir/branch.latx-tb.log"

"$reference" "$workdir/eflags-random" >"$workdir/random.reference"
"$latx" -latx-host-hwcap 0x10 "$workdir/eflags-random" \
    >"$workdir/random.latx"
python3 "$test_dir/compare-latx-eflags-differential.py" \
    "$workdir/random.reference" "$workdir/random.latx"

echo "PASS: QEMU TCG and no-LBT LATX agree on EFLAGS consumers and branches"
