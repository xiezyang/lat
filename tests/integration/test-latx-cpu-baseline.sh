#!/bin/sh
set -eu

if [ "$#" -ne 4 ]; then
    echo "usage: $0 NO_A_LATX AVX_LATX STATIC_X86_PROBE OUTPUT_DIR" >&2
    exit 2
fi

no_a_latx=$1
avx_latx=$2
probe=$3
output_dir=$4
cpu=${LATX_TEST_CPU:-0}

mkdir -p "$output_dir"

run_pinned() {
    taskset -c "$cpu" "$@"
}

run_pinned env -u LATX_AVX_CPUID "$no_a_latx" "$probe" \
    >"$output_dir/no-a.raw" 2>"$output_dir/no-a.stderr"
run_pinned env LATX_AVX_CPUID=0 "$avx_latx" "$probe" \
    >"$output_dir/with-a-hidden.raw" \
    2>"$output_dir/with-a-hidden.stderr"

sed -n '/^LATX_CPU_BASELINE_V1$/,$p' "$output_dir/no-a.raw" \
    >"$output_dir/no-a.guest"
sed -n '/^LATX_CPU_BASELINE_V1$/,$p' \
    "$output_dir/with-a-hidden.raw" >"$output_dir/with-a-hidden.guest"

if ! cmp -s "$output_dir/no-a.guest" \
    "$output_dir/with-a-hidden.guest"; then
    diff -u "$output_dir/no-a.guest" \
        "$output_dir/with-a-hidden.guest" \
        >"$output_dir/cpu-state.diff" || true
    echo "FAIL guest CPU state differs; see $output_dir/cpu-state.diff" >&2
    exit 1
fi

if ! grep -Fq \
    'FEATURES fma=0 xsave=0 osxsave=0 avx=0 f16c=0 avx2=0' \
    "$output_dir/with-a-hidden.guest"; then
    echo "FAIL AVX-related CPUID features remain visible" >&2
    exit 1
fi

if ! grep -Fq 'XGETBV visible=0' \
    "$output_dir/with-a-hidden.guest"; then
    echo "FAIL XGETBV visibility differs from the expected no-a baseline" >&2
    exit 1
fi

sed -n '/^CPUINFO_BEGIN$/,/^CPUINFO_END$/p' \
    "$output_dir/with-a-hidden.guest" >"$output_dir/cpuinfo.txt"
if grep -Eq '(^|[[:space:]])(avx|avx2|fma|f16c|xsave|osxsave)' \
    "$output_dir/cpuinfo.txt"; then
    echo "FAIL AVX-related flag remains in /proc/cpuinfo" >&2
    exit 1
fi

echo "PASS complete guest CPU state matches the no-a baseline"
grep '^FEATURES ' "$output_dir/with-a-hidden.guest"
grep '^XGETBV ' "$output_dir/with-a-hidden.guest"
wc -l -c "$output_dir/no-a.guest" "$output_dir/cpuinfo.txt"
