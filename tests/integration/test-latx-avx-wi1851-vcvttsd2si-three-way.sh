#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
    echo "usage: $0 LATX PROBE EXPECTED_DIR OUTPUT_DIR" >&2
    exit 2
fi

latx=$1
probe=$2
expected=$3
output=$4
script_dir=$(cd "$(dirname "$0")" && pwd)
source_file=$(cd "$script_dir/../.." && pwd)/target/i386/latx/translator/tr-simd-cvt.c
mkdir -p "$output"

LATX_AVX_CPUID_VALUE=1 \
    bash "$script_dir/test-latx-avx-single-vcvttsd2si.sh" verify \
    "$latx" "$probe" "$expected" "$source_file"

addr=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1 }')
[[ -n "$addr" ]]
cases="actual load-cross-1 load-cross-7 load-unreadable fpe-invalid fpe-precision fpe-subnormal-precision no-signal-daz no-signal-old-ie no-signal-old-pe"

for opt in 0 1; do
    for name in $cases; do
        if [[ "$name" == actual ]]; then
            expected_name=reference
            args=actual
        else
            expected_name=$name
            args=$name
        fi
        log="$output/$opt-$name.gdb.log"
        stdout="$output/$opt-$name.stdout"
        stderr="$output/$opt-$name.stderr"
        gdb -q -batch "$latx" \
            -ex 'set pagination off' \
            -ex 'handle SIGSEGV pass nostop noprint' \
            -ex 'handle SIGBUS pass nostop noprint' \
            -ex 'handle SIGFPE pass nostop noprint' \
            -ex 'break translate_context_init' \
            -ex 'set environment LATX_AVX_CPUID 1' \
            -ex "set environment LATX_CVT_OPT $opt" \
            -ex "run $probe $args > $stdout 2> $stderr" \
            -ex "set {int}$addr = 0" \
            -ex "x/wd $addr" \
            -ex continue >"$log" 2>&1 || true
        grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$log"
        if [[ "$expected_name" == reference || "$expected_name" == no-signal-* ]]; then
            grep -Eq 'exited normally' "$log"
        elif [[ "$expected_name" == fpe-* ]]; then
            grep -Eq 'exited with code 0210' "$log"
        else
            grep -Eq 'exited with code 0213' "$log"
        fi
        python3 - "$expected/$expected_name.bin" "$stdout" "$expected_name" <<'PY'
import sys

expected = open(sys.argv[1], "rb").read()
actual = open(sys.argv[2], "rb").read()
name = sys.argv[3]
if name == "reference" or name.startswith("no-signal-"):
    assert expected == actual, (name, len(expected), len(actual))
else:
    assert len(expected) == len(actual), (name, len(expected), len(actual))
    end = 32 if name.startswith("fpe-") else 16
    assert expected[:end] == actual[:end], name
    flags = 32 if name.startswith("fpe-") else 16
    ef = int.from_bytes(expected[flags:flags + 8], "little") & ~0x10000
    af = int.from_bytes(actual[flags:flags + 8], "little") & ~0x10000
    assert ef == af, (name, hex(ef), hex(af))
    tail = 40 if name.startswith("fpe-") else 24
    assert expected[tail:] == actual[tail:], name
PY
    done
done

python3 - "$source_file" <<'PY'
import sys
text = open(sys.argv[1]).read()
start = text.index("translate_vcvttsd2si_lsx")
end = text.index("static bool translate_cvttsx2si_opt", start)
body = text[start:end]
assert "vcvttsd2si_load_scalar" in body
assert "helper_raise_simd_exception" in body
assert "la_xv" not in body
print("PASS VCVTTSD2SI LSX source contract")
PY

echo "PASS VCVTTSD2SI x86/LASX/forced-LSX three-way regression"
