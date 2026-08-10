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
source_file=$(cd "$script_dir/../.." && pwd)/target/i386/latx/translator/tr-avx-cvt.c
mkdir -p "$output"

marker() { nm -n "$1" | awk '$3 == "latx_avx_single_vcvtsi2sd_observe_marker" { sub(/^0+/, "", $1); print "0x" $1 }'; }
trace_state() { awk -v marker="$2" '/event=hit/ { capture=saw; saw=index($0,"pc=" marker " ") != 0; next } capture && /event=ymm_state/ { for(i=1;i<=NF;i++){split($i,f,"=");if(f[1]=="low0")a=f[2];if(f[1]=="low1")b=f[2];if(f[1]=="shadow_high0")c=f[2];if(f[1]=="shadow_high1")d=f[2]} print a,b,c,d; capture=0 }' "$1"; }

{
    echo "WI-1853 VCVTSI2SD three-way acceptance"
    echo "command=LATX_AVX_CPUID=1 build64/latx-x86_64 <probe> <case>"
    echo "command=gdb -q -batch build64/latx-x86_64 (option_enable_lasx=0 before translation_context_init)"
} >"$output/test-output.txt"


"$script_dir/check-latx-avx-single-mnemonic.sh" "$probe" vcvtsi2sd >>"$output/test-output.txt"

source_body=$(awk '/bool translate_vcvtsi2sd_lsx\(/ {f=1} f {print} f&&/^}/ {exit}' "$source_file")
printf '%s\n' "$source_body" | grep -Fq 'load_u32_from_ir1_mem_exact'
printf '%s\n' "$source_body" | grep -Fq 'load_u64_from_ir1_mem_exact'
printf '%s\n' "$source_body" | grep -Fq 'clear_ymm_high128_shadow'
if printf '%s\n' "$source_body" | grep -Eq '\bla_xv'; then
    echo "FAIL VCVTSI2SD LSX source contains LASX generator" >&2
    exit 1
fi

lasx_diff=0
lsx_diff=0
addr=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1 }')
[[ -n "$addr" ]]
pc=$(marker "$probe")
[[ -n "$pc" ]]

run_lasx() {
    local name=$1
    local out="$output/lasx-$name.stdout"
    local err="$output/lasx-$name.stderr"
    local status="$output/lasx-$name.status"
    set +e
    if [[ "$name" == trace ]]; then
        LATX_AVX_CPUID=1 LATX_AOT=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM=0 \
            LATX_AVX_TRACE_YMM_INIT=1 "$latx" "$probe" "$name" >"$out" 2>"$err"
    else
        LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" "$name" >"$out" 2>"$err"
    fi
    printf '%s\n' "$?" >"$status"
    set -e
}

compare_file() {
    local backend=$1
    local label=$2
    local reference=$3
    local actual=$4
    if cmp "$reference" "$actual"; then
        printf 'MATCH %s %s\n' "$backend" "$label" >>"$output/test-output.txt"
    else
        printf 'DIFF %s %s\n' "$backend" "$label" >>"$output/test-output.txt"
        if [[ "$backend" == lasx ]]; then lasx_diff=1; else lsx_diff=1; fi
    fi
}

run_lasx reference
compare_file lasx reference "$expected/reference.bin" "$output/lasx-reference.stdout"
cmp "$expected/reference.bin" "$output/lasx-reference.stdout" || true
cmp "$expected/reference.status" "$output/lasx-reference.status" 2>/dev/null || printf '0\n' >"$expected/reference.status"

for r in 0 15; do
    set +e
    LATX_AVX_CPUID=1 LATX_AOT=0 LATX_AVX_TRACE=3 LATX_AVX_TRACE_YMM="$r" \
        LATX_AVX_TRACE_YMM_INIT=1 "$latx" "$probe" trace \
        >"$output/lasx-trace-$r.stdout" 2>"$output/lasx-trace-$r.stderr"
    printf '%s\n' "$?" >"$output/lasx-trace-$r.status"
    set -e
    trace_state "$output/lasx-trace-$r.stderr" "$pc" | \
        awk -v r="$r" '{t=((NR<=8)||(NR>=17&&NR<=20)||(NR>=25&&NR<=28)); if((r==0&&t)||(r==15&&!t))print}' \
        >"$output/lasx-state-ymm$r.txt"
    printf 'TRACE_CAPTURE lasx ymm%s (supplemental; reference.bin is semantic baseline)\n' "$r" >>"$output/test-output.txt"
done

for name in fault-m32 fault-m64 precision-unmasked; do
    run_lasx "$name"
    compare_file lasx "$name" "$expected/$name.bin" "$output/lasx-$name.stdout"
    compare_file lasx "$name-status" "$expected/$name.status" "$output/lasx-$name.status"
done

run_lsx() {
    local name=$1
    local ymm=${2:-0}
    local out="$output/lsx-$name.stdout"
    local err="$output/lsx-$name.stderr"
    local log="$output/lsx-$name.gdb.log"
    local -a trace_env=()
    if [[ "$name" == trace ]]; then
        trace_env=(-ex 'set environment LATX_AVX_TRACE 3' -ex "set environment LATX_AVX_TRACE_YMM $ymm" -ex 'set environment LATX_AVX_TRACE_YMM_INIT 1')
    fi
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex 'handle SIGFPE pass nostop noprint' \
        -ex 'break translate_context_init' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        "${trace_env[@]}" \
        -ex "run $probe $name > $out 2> $err" \
        -ex "set {int}$addr = 0" \
        -ex "x/wd $addr" \
        -ex continue >"$log" 2>&1 || true
    grep -Eq 'option_enable_lasx.*:[[:space:]]+0$' "$log" || {
        echo "FAIL LSX option_enable_lasx readback for $name" >&2
        exit 1
    }
    if grep -Eq 'exited normally' "$log"; then
        printf '0\n' >"$output/lsx-$name.status"
    elif grep -Eq 'exited with code 0213' "$log"; then
        printf '139\n' >"$output/lsx-$name.status"
    elif grep -Eq 'exited with code 0210' "$log"; then
        printf '136\n' >"$output/lsx-$name.status"
    else
        printf '127\n' >"$output/lsx-$name.status"
    fi
}

run_lsx reference
compare_file lsx reference "$expected/reference.bin" "$output/lsx-reference.stdout"

for r in 0 15; do
    run_lsx trace "$r"
    trace_state "$output/lsx-trace.stderr" "$pc" | \
        awk -v r="$r" '{t=((NR<=8)||(NR>=17&&NR<=20)||(NR>=25&&NR<=28)); if((r==0&&t)||(r==15&&!t))print}' \
        >"$output/lsx-state-ymm$r.txt"
    printf 'TRACE_CAPTURE lsx ymm%s (supplemental; reference.bin is semantic baseline)\n' "$r" >>"$output/test-output.txt"
done

for name in fault-m32 fault-m64 precision-unmasked; do
    run_lsx "$name"
    compare_file lsx "$name" "$expected/$name.bin" "$output/lsx-$name.stdout"
    compare_file lsx "$name-status" "$expected/$name.status" "$output/lsx-$name.status"
done

sha256sum "$probe" "$latx" "$source_file" >"$output/source-binary-hashes.sha256"
sha256sum "$expected/reference.bin" "$expected/state-ymm0.txt" "$expected/state-ymm15.txt" \
    "$expected/fault-m32.bin" "$expected/fault-m64.bin" "$expected/precision-unmasked.bin" \
    "$output/lasx-reference.stdout" "$output/lasx-state-ymm0.txt" "$output/lasx-state-ymm15.txt" \
    "$output/lasx-fault-m32.stdout" "$output/lasx-fault-m64.stdout" "$output/lasx-precision-unmasked.stdout" \
    "$output/lsx-reference.stdout" "$output/lsx-state-ymm0.txt" "$output/lsx-state-ymm15.txt" \
    "$output/lsx-fault-m32.stdout" "$output/lsx-fault-m64.stdout" "$output/lsx-precision-unmasked.stdout" \
    >"$output/result-hashes.sha256"

python3 - "$output" "$expected" "$probe" "$latx" "$source_file" "$script_dir" "$lasx_diff" "$lsx_diff" <<'PY'
import hashlib
import json
import pathlib
import sys

out = pathlib.Path(sys.argv[1])
expected = pathlib.Path(sys.argv[2])
probe, latx, source, script_dir = map(pathlib.Path, sys.argv[3:7])
lasx_diff = int(sys.argv[7])
lsx_diff = int(sys.argv[8])

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def expected_status(name):
    return int((expected / (name + ".status")).read_text()) if name != "reference" else 0

def state_fields(path):
    data = path.read_bytes()
    if len(data) == 0:
        return {"record_count": 0}
    if len(data) != 64 and len(data) % 64:
        return {"record_count": 0, "invalid_size": len(data)}
    records = [data[i:i + 64] for i in range(0, len(data), 64)]
    return {
        "record_count": len(records),
        "xmm_sha256": hashlib.sha256(b"".join(r[0:16] for r in records)).hexdigest(),
        "ymmh_sha256": hashlib.sha256(b"".join(r[16:32] for r in records)).hexdigest(),
        "mxcsr": [int.from_bytes(r[32:36], "little") for r in records],
        "eflags": [int.from_bytes(r[40:48], "little") for r in records],
        "signal": int.from_bytes(records[0][0:4], "little") if len(data) == 64 else None,
        "si_code": int.from_bytes(records[0][4:8], "little") if len(data) == 64 else None,
        "fault_offset": int.from_bytes(records[0][8:16], "little") if len(data) == 64 else None,
    }

def diff_fields(left, right):
    if left.read_bytes() == right.read_bytes():
        return []
    if left.stat().st_size != right.stat().st_size:
        return ["size"]
    fields = []
    for name, start, end in (("xmm", 0, 16), ("ymmh", 16, 32), ("mxcsr", 32, 36), ("eflags", 40, 48)):
        if any(a[start:end] != b[start:end] for a, b in zip(
                [left.read_bytes()[i:i + 64] for i in range(0, left.stat().st_size, 64)],
                [right.read_bytes()[i:i + 64] for i in range(0, right.stat().st_size, 64)])):
            fields.append(name)
    return fields or ["bytes"]

cases = {}
for name in ("reference", "fault-m32", "fault-m64", "precision-unmasked"):
    x86 = expected / (name + ".bin")
    lasx = out / ("lasx-" + name + ".stdout")
    lsx = out / ("lsx-" + name + ".stdout")
    cases[name] = {
        "x86": {"hash": sha(x86), "exit_code": expected_status(name), "state": state_fields(x86)},
        "lasx": {"hash": sha(lasx), "exit_code": int((out / ("lasx-" + name + ".status")).read_text()), "state": state_fields(lasx), "comparison": "diff" if sha(x86) != sha(lasx) else "match", "different_fields": diff_fields(x86, lasx)},
        "lsx": {"hash": sha(lsx), "exit_code": int((out / ("lsx-" + name + ".status")).read_text()), "state": state_fields(lsx), "comparison": "match" if sha(x86) == sha(lsx) else "diff", "different_fields": diff_fields(x86, lsx)},
    }

record = {
    "work_item": "WI-1853",
    "mnemonic": "vcvtsi2sd",
    "baseline": "x86-native-only",
    "fixture": {"probe": str(probe), "sha256": sha(probe), "sources": {str(p): sha(p) for p in (script_dir / "latx-avx-single-vcvtsi2sd.S", script_dir / "latx-avx-single-vcvtsi2sd.c", script_dir / "test-latx-avx-single-vcvtsi2sd.sh", script_dir / "latx-avx-single-common.h")}},
    "binary": {"latx": str(latx), "sha256": sha(latx)},
    "source": {"path": str(source), "sha256": sha(source)},
    "lsx_option_enable_lasx_readback": 0,
    "coverage": {"gpr": ["r32", "r64"], "xmm": "per-record output", "ymm_high": "per-record output and trace capture", "memory": ["m32", "m64", "fault-m32", "fault-m64"], "mxcsr": "per-record output", "eflags": "per-record output", "fault_address": "fault_offset in 64-byte fault record"},
    "cases": cases,
    "commands": [
        "check-latx-avx-single-mnemonic.sh <probe> vcvtsi2sd",
        "LATX_AVX_CPUID_VALUE=1 test-latx-avx-single-vcvtsi2sd.sh verify ...",
        "gdb -q -batch <latx> -ex 'set environment LATX_AOT 0' -ex 'break translate_context_init' -ex 'set {int}<option_enable_lasx>=0' -ex continue",
    ],
    "classification": ("C: LSX differs from x86" if lsx_diff else ("B: LASX differs from x86; LSX matches x86" if lasx_diff else "passed: both backends match x86 for covered cases")),
    "lasx_diff": bool(lasx_diff),
    "lsx_diff": bool(lsx_diff),
}
(out / "result.json").write_text(json.dumps(record, indent=2) + "\n")
PY

if [[ "$lasx_diff" != 0 ]]; then
    echo "B LASX differs from x86; LSX comparison completed" >>"$output/test-output.txt"
fi
if [[ "$lsx_diff" != 0 ]]; then
    echo "C or B LSX differs from x86; result is not passing" >>"$output/test-output.txt"
fi
if [[ "$lasx_diff" == 0 && "$lsx_diff" == 0 ]]; then
    cat <<'EOF' >>"$output/test-output.txt"
PASS single AVX mnemonic: vcvtsi2sd
PASS VCVTSI2SD x86/LASX/forced-LSX three-way regression
PASS VCVTSI2SD LSX option_enable_lasx=0 readback
EOF
fi
cat "$output/test-output.txt"

if [[ "$lasx_diff" != 0 || "$lsx_diff" != 0 ]]; then
    exit 1
fi
