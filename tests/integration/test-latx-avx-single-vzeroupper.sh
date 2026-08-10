#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 LATX_X86_64 PROBE X86_REFERENCE" >&2
    exit 2
fi

latx=$1
probe=$2
reference=$3
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

[ -x "$latx" ] || {
    echo "FAIL LATX binary is not executable: $latx" >&2
    exit 2
}
[ -x "$probe" ] || {
    echo "FAIL probe is not executable: $probe" >&2
    exit 2
}
[ -f "$reference" ] || {
    echo "FAIL x86 reference is not a file: $reference" >&2
    exit 2
}

mnemonics=$(objdump -d --no-show-raw-insn -Mintel "$probe" |
    awk '$2 ~ /^v/ { print $2 }' | sort -u)
[ "$mnemonics" = "vzeroupper" ] || {
    echo "FAIL probe AVX mnemonics: $mnemonics" >&2
    exit 1
}
echo "PASS probe contains only vzeroupper"

option_address=$(nm -g "$latx" | awk '$3 == "option_enable_lasx" {
    print "0x" $1
}')
[ -n "$option_address" ] || {
    echo "FAIL option_enable_lasx symbol is missing" >&2
    exit 1
}

run_lasx()
{
    set +e
    env LATX_AVX_CPUID=1 "$latx" "$probe" >"$tmpdir/lasx.out" \
        2>"$tmpdir/lasx.err"
    status=$?
    set -e
    [ "$status" -eq 0 ] || {
        echo "FAIL LASX status=$status" >&2
        cat "$tmpdir/lasx.err" >&2
        exit 1
    }
}

run_lsx()
{
    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'break translate_context_init' \
        -ex "run $probe > $tmpdir/lsx.out 2> $tmpdir/lsx.err" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue >"$tmpdir/lsx.gdb" 2>&1
    gdb_status=$?
    set -e
    [ "$gdb_status" -eq 0 ] || {
        echo "FAIL LSX GDB status=$gdb_status" >&2
        cat "$tmpdir/lsx.gdb" >&2
        exit 1
    }
    grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$tmpdir/lsx.gdb" || {
        echo "FAIL LSX option_enable_lasx was not 0" >&2
        cat "$tmpdir/lsx.gdb" >&2
        exit 1
    }
    grep -Fq 'exited normally' "$tmpdir/lsx.gdb" || {
        echo "FAIL LSX inferior did not exit normally" >&2
        cat "$tmpdir/lsx.gdb" >&2
        exit 1
    }
}

run_lasx
run_lsx
cmp "$reference" "$tmpdir/lasx.out"
cmp "$reference" "$tmpdir/lsx.out"
printf 'PASS x86/LASX/LSX VZEROUPPER output matches (%s bytes)\n' \
    "$(wc -c <"$reference")"
sha256sum "$probe" "$latx" "$reference"
echo "PASS LSX option_enable_lasx=0"
