#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
set -euo pipefail

if [[ $# != 4 ]]; then
    echo "Usage: $0 LATX RUNTIME SDK NEW_RESULT_DIRECTORY" >&2
    exit 2
fi
latx=$(realpath "$1")
runtime=$(realpath "$2")
sdk=$(realpath "$3")
result=$(realpath -m "$4")
[[ -x "$latx" && -x "$sdk/aikit_test" && -d "$runtime" ]]
# Refuse reuse: neither cached translations nor previous results may leak in.
mkdir -- "$result"
mkdir -- "$result/home"
export HOME="$result/home" LBT_X86=0 LATX_AOT=1
export LATX_XTTS_HOT_TU=1 LATX_XTTS_DECRYPT=1
export LATX_XTTS_CLOCK_NANOSLEEP=1 LATX_XTTS_MATRIX=1
export LD_LIBRARY_PATH=./libs
unset LATX_SOFTFPU LATX_PERF_MAP LATX_TU_SCAN LATX_AOT_SCAN
unset LATX_DUMP LATX_SHOW_TB LAT_LOG LAT_LOG_FILENAME
expected=d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac
cd "$sdk"
{
    date -Is
    printf 'binary=%s\nruntime=%s\nsdk=%s\nHOME=%s\n' \
        "$latx" "$runtime" "$sdk" "$HOME"
    sha256sum "$latx" ./aikit_test
    find ./libs -type f -name '*.so*' -exec sha256sum {} +
    env | LC_ALL=C sort | grep -E '^(LATX_|LBT_X86=|LD_LIBRARY_PATH=)'
} >"$result/provenance.txt"
sha256sum "$latx" >"$result/binary.sha256"
if [[ -e output.pcm || -L output.pcm ]]; then
    mv -- output.pcm "$result/previous-output.pcm"
fi
TIMEFORMAT='real=%3R user=%3U sys=%3S'
for phase in jit generate load warm1 warm2 warm3 run1 run2 run3 run4 run5; do
    export LATX_AOT=1
    unset LATX_AOT_SCAN
    [[ "$phase" != jit ]] || export LATX_AOT=0
    if [[ "$phase" == generate || "$phase" == load ]]; then
        export LATX_AOT_SCAN=1
    fi
    rc=0
    { time timeout -k 2 35 "$latx" -L "$runtime" ./aikit_test \
        >"$result/$phase.out" 2>"$result/$phase.err"; } \
        2>"$result/$phase.time" || rc=$?
    printf '%s rc=%d %s\n' "$phase" "$rc" "$(<"$result/$phase.time")" \
        | tee -a "$result/times.txt"
    [[ $rc == 0 && -f output.pcm && ! -L output.pcm ]]
    printf '%s  output.pcm\n' "$expected" | sha256sum -c - \
        | tee "$result/$phase.pcm-check"
    mv -- output.pcm "$result/$phase.pcm"
    if [[ "$phase" == generate ]]; then
        ! grep -q 'AOT_SCAN_LOAD_SUCCESS' "$result/$phase.err"
        grep -q 'AOT_SCAN_GENERATE_SUCCESS' "$result/$phase.err"
    elif [[ "$phase" == load ]]; then
        grep -q 'AOT_SCAN_LOAD_SUCCESS' "$result/$phase.err"
        grep -q 'AOT_SCAN_RECOVER' "$result/$phase.err"
    fi
done
sha256sum -c "$result/binary.sha256"
awk '/^run[1-5] / {split($3,a,"="); n++; sum+=a[2]; v[n]=a[2]}
    END {for(i=1;i<=n;i++)for(j=i+1;j<=n;j++)if(v[j]<v[i]){
        t=v[i];v[i]=v[j];v[j]=t}
    if(n==5)printf "Five measured runs: mean=%.3fs median=%.3fs\n",sum/n,v[3]}' \
    "$result/times.txt" | tee "$result/summary.txt"
echo "PASS: fresh JIT, fresh AOT generation/load, and all PCM outputs"
