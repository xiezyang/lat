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
source_file=$(cd "$script_dir/../.." && pwd)/target/i386/latx/translator/tr-avx-mov.c
mkdir -p "$output"
lasx_diff=0
lsx_diff=0

"$script_dir/check-latx-avx-single-mnemonic.sh" "$probe" vmovd
body=$(awk '/^bool translate_vmovd_lsx\(/ { found = 1 } found { print } found && /^}/ { exit }' "$source_file")
printf '%s\n' "$body" | grep -Fq 'load_u32_from_ir1_mem_exact'
printf '%s\n' "$body" | grep -Fq 'store_u32_to_ir1_mem_exact'
printf '%s\n' "$body" | grep -Fq 'clear_ymm_high128_shadow'
if printf '%s\n' "$body" | grep -Eq '\bla_xv'; then
    echo "FAIL VMOVD LSX source contains LASX generator" >&2
    exit 1
fi

run_lasx() {
    local name=$1
    local out="$output/lasx-$name.stdout"
    local err="$output/lasx-$name.stderr"
    local status="$output/lasx-$name.status"
    set +e
    if [[ "$name" == reference ]]; then
        LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" >"$out" 2>"$err"
    else
        LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" "$name" >"$out" 2>"$err"
    fi
    printf '%s\n' "$?" >"$status"
    set -e
}

compare_payload() {
    local name=$1
    local actual=$2
    local backend=$3
    local mismatch=0
    local reference="$expected/$name.bin"

    if [[ "$name" == reference ]]; then
        [[ "$(wc -c <"$reference")" == "$(wc -c <"$actual")" ]] || mismatch=1
        cmp -n 4800 "$reference" "$actual" || mismatch=1
        for record in 0 1 2 3 4 5 6; do
            offset=$((4800 + record * 56))
            cmp -n 24 -i "$offset:$offset" "$reference" "$actual" || mismatch=1
            cmp -n 16 -i "$((offset + 40)):$((offset + 40))" "$reference" "$actual" || mismatch=1
        done
    else
        cmp -n 24 -i 0:0 "$reference" "$actual" || mismatch=1
        cmp -n 16 -i 40:40 "$reference" "$actual" || mismatch=1
    fi
    if [[ "$mismatch" != 0 ]]; then
        if [[ "$backend" == LASX ]]; then lasx_diff=1; else lsx_diff=1; fi
        echo "DIFF $backend $name" >>"$output/test-output.txt"
    else
        echo "MATCH $backend $name (signal-frame YMMH excluded)" >>"$output/test-output.txt"
    fi
}

compare_status() {
    local name=$1
    local backend=$2
    [[ "$name" == reference ]] && return 0
    if ! cmp "$expected/$name.status" "$output/${backend,,}-$name.status"; then
        if [[ "$backend" == LASX ]]; then lasx_diff=1; else lsx_diff=1; fi
    fi
}

run_lasx reference
compare_payload reference "$output/lasx-reference.stdout" LASX
for name in fault-load-cross-1 fault-load-cross-2 fault-load-cross-3; do
    run_lasx "$name"
    compare_payload "$name" "$output/lasx-$name.stdout" LASX
    compare_status "$name" LASX
done

addr=$(nm -n "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1 }')
[[ -n "$addr" ]]

run_lsx() {
    local name=$1
    local log="$output/lsx-$name.gdb.log"
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex 'break translate_context_init' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        -ex "run $probe ${name/reference/} > $output/lsx-$name.stdout 2> $output/lsx-$name.stderr" \
        -ex "set {int}$addr = 0" \
        -ex "x/wd $addr" \
        -ex continue >"$log" 2>&1 || true
    grep -Eq 'option_enable_lasx.*:[[:space:]]+0$' "$log" || exit 1
}

run_lsx reference
printf '0\n' >"$output/lsx-reference.status"
compare_payload reference "$output/lsx-reference.stdout" LSX
for name in fault-load-cross-1 fault-load-cross-2 fault-load-cross-3; do
    run_lsx "$name"
    grep -Eq 'exited with code 0213' "$output/lsx-$name.gdb.log"
    printf '139\n' >"$output/lsx-$name.status"
    compare_payload "$name" "$output/lsx-$name.stdout" LSX
    compare_status "$name" LSX
done

sha256sum "$probe" "$latx" "$source_file" "$expected/reference.bin" \
    "$output/lasx-reference.stdout" "$output/lsx-reference.stdout" \
    "$expected/fault-load-cross-1.bin" "$output/lasx-fault-load-cross-1.stdout" \
    "$output/lsx-fault-load-cross-1.stdout" >"$output/hashes.sha256"

python3 - "$output" "$expected" "$probe" "$latx" "$source_file" "$lasx_diff" "$lsx_diff" <<'PY'
import hashlib
import json
import pathlib
import sys

out, expected = map(pathlib.Path, sys.argv[1:3])
probe, latx, source = map(pathlib.Path, sys.argv[3:6])
lasx_diff, lsx_diff = map(int, sys.argv[6:8])

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def case(name):
    x86 = expected / (name + ".bin")
    lasx = out / ("lasx-" + name + ".stdout")
    lsx = out / ("lsx-" + name + ".stdout")
    return {
        "x86": {"hash": sha(x86), "exit_code": int((expected / (name + ".status")).read_text()) if name != "reference" else 0},
        "lasx": {"hash": sha(lasx), "exit_code": int((out / ("lasx-" + name + ".status")).read_text()), "comparison": "match_except_signal_frame_ymmh" if not lasx_diff else "diff"},
        "lsx": {"hash": sha(lsx), "exit_code": 0 if name == "reference" else 139, "comparison": "match_except_signal_frame_ymmh" if not lsx_diff else "diff"},
    }

record = {
    "work_item": "WI-1855",
    "mnemonic": "vmovd",
    "baseline": "x86-native-only",
    "fixture": {"probe": str(probe), "sha256": sha(probe)},
    "source": {"path": str(source), "sha256": sha(source)},
    "binary": {"path": str(latx), "sha256": sha(latx)},
    "cases": {name: case(name) for name in ("reference", "fault-load-cross-1", "fault-load-cross-2", "fault-load-cross-3")},
    "lsx_option_enable_lasx_readback": 0,
    "commands": [
        "check-latx-avx-single-mnemonic.sh <probe> vmovd",
        "LATX_AVX_CPUID=1 LATX_AOT=0 <latx> <probe> [fault-load-cross-N]",
        "gdb -q -batch <latx> -ex 'set environment LATX_AOT 0' -ex 'set {int}<option_enable_lasx>=0' -ex continue",
    ],
    "classification": "passed: LASX and LSX match x86" if not (lasx_diff or lsx_diff) else ("B: LASX differs from x86; LSX matches x86" if not lsx_diff else "B/C: LSX differs from x86"),
    "lasx_diff": bool(lasx_diff),
    "lsx_diff": bool(lsx_diff),
    "fixture_limit": "LSX signal-frame YMMH is excluded from the VMOVD payload comparison; XMM, memory, status and fault fields are compared.",
}
(out / "result.json").write_text(json.dumps(record, indent=2) + "\n")
PY

cat <<EOF >"$output/test-output.txt"
WI-1855 VMOVD three-way acceptance
PASS single AVX mnemonic: vmovd
PASS LSX option_enable_lasx=0 readback
EOF
if [[ "$lasx_diff" != 0 ]]; then echo "B LASX differs from x86" >>"$output/test-output.txt"; fi
if [[ "$lsx_diff" != 0 ]]; then echo "B/C LSX differs from x86" >>"$output/test-output.txt"; fi
if [[ "$lasx_diff" == 0 && "$lsx_diff" == 0 ]]; then echo "PASS VMOVD x86/LASX/forced-LSX regression with LATX_AOT=0" >>"$output/test-output.txt"; fi
cat "$output/test-output.txt"
if [[ "$lasx_diff" != 0 || "$lsx_diff" != 0 ]]; then exit 1; fi
