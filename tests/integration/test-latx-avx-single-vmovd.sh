#!/bin/sh
set -eu

if [ "$#" -ne 4 ]; then
    echo "usage: $0 LATX_X86_64 STATIC_X86_PROBE X86_REFERENCE SOURCE" >&2
    exit 2
fi

latx=$1
probe=$2
reference=$3
source=$4
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

if [ ! -x "$latx" ] || [ ! -x "$probe" ] ||
   [ ! -f "$reference" ] || [ ! -f "$source" ]; then
    echo "FAIL LATX, probe, x86 reference, or source is missing" >&2
    exit 2
fi

if [ "${LATX_AVX_SKIP_PROBE_CHECK:-0}" = 1 ]; then
    echo "SKIP single AVX mnemonic check: x86 objdump unavailable"
else
    "$(dirname "$0")/check-latx-avx-single-mnemonic.sh" "$probe" vmovd
fi

body=$(awk '
    /^bool translate_vmovd\(IR1_INST \*pir1\)/ { found = 1 }
    found { print }
    found && /^}/ { exit }
' "$source")
if ! printf '%s\n' "$body" | grep -Fq 'if (1)' ||
   ! printf '%s\n' "$body" | grep -Fq '/* LSX-only path */' ||
   ! printf '%s\n' "$body" | grep -Fq '/* Original LASX path */'; then
    echo "FAIL translate_vmovd branch layout" >&2
    exit 1
fi

lsx_path=$(printf '%s\n' "$body" | awk '
    /\/\* LSX-only path \*\// { found = 1; next }
    /\/\* Original LASX path \*\// { exit }
    found { print }
')
if [ -z "$lsx_path" ] || printf '%s\n' "$lsx_path" | grep -Eq '\bla_xv'; then
    echo "FAIL translate_vmovd reachable path contains LASX" >&2
    exit 1
fi
for required in la_vxor_v la_vinsgr2vr_w la_vpickve2gr_wu \
                load_u32_from_ir1_mem_exact store_u32_to_ir1_mem_exact \
                clear_ymm_high128_shadow; do
    if ! printf '%s\n' "$lsx_path" | grep -Fq "$required"; then
        echo "FAIL translate_vmovd reachable path misses $required" >&2
        exit 1
    fi
done

for helper in load_u32_from_ir1_mem_exact store_u32_to_ir1_mem_exact; do
    helper_body=$(awk -v symbol="$helper" '
        $0 ~ symbol "\\(" { found = 1 }
        found { print }
        found && /^}/ { exit }
    ' target/i386/latx/translator/tr-opnd-process.c)
    if [ -z "$helper_body" ] ||
       ! printf '%s\n' "$helper_body" | grep -Fq 'check_guest_mem_range(address, 4' ||
       printf '%s\n' "$helper_body" | grep -Eq '\bla_xv'; then
        echo "FAIL $helper is not an exact non-LASX m32 helper" >&2
        exit 1
    fi
done

dump=$(objdump -d "$latx" | awk '
    /<translate_vmovd>:/ { found = 1 }
    found { print }
    found && /^$/ { exit }
')
if ! printf '%s\n' "$dump" | grep -Fq '<translate_vmovd>';
then
    echo "FAIL compiled translate_vmovd symbol is missing" >&2
    exit 1
fi
if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>';
then
    echo "FAIL compiled translate_vmovd can call LASX generators" >&2
    printf '%s\n' "$dump" | grep -E '<la_xv[^>]*>' >&2
    exit 1
fi

expected_size=5192
actual_reference_size=$(wc -c <"$reference")
if [ "$actual_reference_size" -ne "$expected_size" ]; then
    echo "FAIL vmovd reference size: $actual_reference_size" >&2
    exit 1
fi

actual=$tmpdir/latx-vmovd.out
env LATX_AVX_CPUID=0 "$latx" "$probe" >"$actual"
if [ "$(wc -c <"$actual")" -ne "$expected_size" ]; then
    echo "FAIL vmovd LATX output size: $(wc -c <"$actual")" >&2
    exit 1
fi
# Each 800-byte normal record contains two RFLAGS/MXCSR snapshots at
# 656..687.  The probe rejects a VMOVD-induced change before writing output,
# so compare the remaining bytes across hosts with different entry flags.
normal_record_size=800
normal_prefix_size=656
normal_suffix_offset=688
normal_suffix_size=112
normal_record=0
while [ "$normal_record" -lt 6 ]; do
    normal_offset=$((normal_record * normal_record_size))
    if ! cmp -n "$normal_prefix_size" -i "$normal_offset:$normal_offset" \
        "$reference" "$actual" ||
       ! cmp -n "$normal_suffix_size" \
        -i "$((normal_offset + normal_suffix_offset)):$(($normal_offset + normal_suffix_offset))" \
        "$reference" "$actual"; then
        echo "FAIL vmovd normal record $normal_record differs" >&2
        exit 1
    fi
    normal_record=$((normal_record + 1))
done

record_size=56
record_offset=4800
record_count=7
record=0
while [ "$record" -lt "$record_count" ]; do
    if ! cmp -n 24 -i "$record_offset:$record_offset" \
        "$reference" "$actual" ||
       ! cmp -n 16 -i "$((record_offset + 40)):$((record_offset + 40))" \
        "$reference" "$actual"; then
        echo "FAIL vmovd fault record $record differs" >&2
        exit 1
    fi
    record=$((record + 1))
    record_offset=$((record_offset + record_size))
done

expected_high=$(od -An -v -tx8 -j 4824 -N 16 "$reference" |
    awk '{ print $1, $2 }')
if [ "$expected_high" != \
     "1122334455667788 99aabbccddeeff00" ]; then
    echo "FAIL unexpected x86 fault YMM high seed: $expected_high" >&2
    exit 1
fi
for crossing in 1 2 3; do
    fault_output=$tmpdir/fault-load-$crossing.out
    fault_trace=$tmpdir/fault-load-$crossing.trace
    set +e
    env LATX_AVX_CPUID=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=0 \
        LATX_AVX_TRACE_YMM_INIT=1 \
        "$latx" "$probe" "fault-load-cross-$crossing" \
        >"$fault_output" 2>"$fault_trace"
    fault_status=$?
    set -e
    if [ "$fault_status" -ne 139 ] ||
       [ "$(wc -c <"$fault_output")" -ne "$record_size" ]; then
        echo "FAIL vmovd load-cross-$crossing status/record" >&2
        exit 1
    fi
    reference_offset=$((4800 + (crossing - 1) * 2 * record_size))
    if ! cmp -n 24 -i "$reference_offset:0" \
        "$reference" "$fault_output" ||
       ! cmp -n 16 -i "$((reference_offset + 40)):40" \
        "$reference" "$fault_output"; then
        echo "FAIL vmovd load-cross-$crossing XMM/memory" >&2
        exit 1
    fi
    trace_state=$(awk '
        /event=ymm_state/ {
            low0 = low1 = high0 = high1 = ""
            for (i = 1; i <= NF; ++i) {
                split($i, field, "=")
                if (field[1] == "low0") low0 = field[2]
                if (field[1] == "low1") low1 = field[2]
                if (field[1] == "shadow_high0") high0 = field[2]
                if (field[1] == "shadow_high1") high1 = field[2]
            }
            print low0, low1, high0, high1
        }
    ' "$fault_trace")
    if [ "$trace_state" != \
         "a7a7a7a7a7a7a7a7 a7a7a7a7a7a7a7a7 1122334455667788 99aabbccddeeff00" ]; then
        echo "FAIL vmovd load-cross-$crossing YMM before fault: $trace_state" >&2
        exit 1
    fi
done

echo "PASS vmovd GPR/XMM, m32, clearing and fault differential"
sha256sum "$reference" "$actual"
