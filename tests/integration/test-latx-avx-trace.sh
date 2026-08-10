#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 LATX_X86_64 STATIC_X86_PROBE" >&2
    exit 2
fi

latx=$1
probe=$2
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

run_trace() {
    cpuid=$1
    trace=$2
    mode=$3
    output=$4

    env LATX_AVX_CPUID="$cpuid" LATX_AVX_TRACE="$trace" \
        "$latx" "$probe" "$mode" >"$output" 2>&1
}

assert_contains() {
    output=$1
    expected=$2
    label=$3

    if ! grep -Fq "$expected" "$output"; then
        echo "FAIL $label: missing '$expected'" >&2
        sed -n '1,160p' "$output" >&2
        exit 1
    fi
    echo "PASS $label"
}

assert_no_hit() {
    output=$1
    label=$2

    if grep -Fq 'LATX_AVX_TRACE event=hit' "$output"; then
        echo "FAIL $label: unexpected AVX hit" >&2
        sed -n '1,160p' "$output" >&2
        exit 1
    fi
    echo "PASS $label"
}

guarded=$tmpdir/guarded.log
unconditional=$tmpdir/unconditional.log
bmi2=$tmpdir/bmi2.log
feature_on=$tmpdir/feature-on.log
disabled=$tmpdir/disabled.log
trap_first=$tmpdir/trap-first.log

run_trace 0 1 guarded "$guarded"
assert_contains "$guarded" 'guarded_avx=skipped' guarded_skips_avx
assert_contains "$guarded" \
    'event=process_summary pid=' guarded_has_summary
assert_contains "$guarded" 'unique=0 total=0' guarded_zero_hits
assert_no_hit "$guarded" guarded_no_hit

run_trace 0 1 unconditional "$unconditional"
assert_contains "$unconditional" \
    'unconditional_avx=executed result=2' unconditional_executes
assert_contains "$unconditional" 'unique=5 total=5' unconditional_five_hits
assert_contains "$unconditional" 'width=256' unconditional_ymm_hit
assert_contains "$unconditional" 'width=64' unconditional_scalar_hit
if grep -F 'LATX_AVX_TRACE event=hit' "$unconditional" |
    grep -Fvq 'class=a-only'; then
    echo "FAIL unconditional_a_only: hit outside -a translation table" >&2
    sed -n '1,160p' "$unconditional" >&2
    exit 1
fi
echo "PASS unconditional_a_only"

run_trace 0 1 bmi2 "$bmi2"
assert_contains "$bmi2" 'bmi2=executed result=7' bmi2_executes
assert_contains "$bmi2" 'unique=0 total=0' bmi2_zero_hits
assert_no_hit "$bmi2" bmi2_not_reported_as_avx

run_trace 1 1 guarded "$feature_on"
assert_contains "$feature_on" 'event=xgetbv' feature_on_records_xgetbv
assert_contains "$feature_on" 'allowed=1' feature_on_xgetbv_allowed
assert_contains "$feature_on" \
    'encoding=legacy class=a-only width=0 opcode=1309 bytes=0f01d0' \
    xgetbv_is_a_only_translation
assert_contains "$feature_on" \
    'guarded_avx=executed result=2' feature_on_guarded_executes

run_trace 0 0 unconditional "$disabled"
if grep -Fq 'LATX_AVX_TRACE' "$disabled"; then
    echo "FAIL trace_disabled: trace output remains" >&2
    sed -n '1,160p' "$disabled" >&2
    exit 1
fi
echo "PASS trace_disabled"

set +e
run_trace 0 2 unconditional "$trap_first"
trap_status=$?
set -e
if [ "$trap_status" -ne 132 ]; then
    echo "FAIL trap_first: expected exit 132, got $trap_status" >&2
    sed -n '1,160p' "$trap_first" >&2
    exit 1
fi
assert_contains "$trap_first" 'event=hit' trap_first_records_hit
echo "PASS trap_first_exit_132"

grep -F 'event=process_summary' "$guarded"
grep -F 'event=process_summary' "$unconditional"
grep -F 'event=process_summary' "$bmi2"
grep -m 1 -F 'event=xgetbv' "$feature_on"
grep -m 1 -F 'event=hit' "$trap_first"
