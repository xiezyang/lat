#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
set -euo pipefail
[[ $# == 6 ]] || { echo "Usage: $0 LATX RUNTIME SDK VALIDATED_HOME NEW_RESULT COUNT" >&2; exit 2; }
latx=$(realpath "$1")
runtime=$(realpath "$2")
sdk=$(realpath "$3")
export HOME=$(realpath "$4")
result=$(realpath -m "$5")
count=$6
[[ $count =~ ^[1-9][0-9]*$ && -d $HOME/.cache/latx ]]
[[ -f $HOME/../binary.sha256 ]]
[[ $(sha256sum "$latx" | cut -d ' ' -f 1) == \
   $(cut -d ' ' -f 1 "$HOME/../binary.sha256") ]]
mkdir -- "$result"
export LBT_X86=0 LATX_AOT=1 LD_LIBRARY_PATH=./libs
unset LATX_SOFTFPU LATX_PERF_MAP LATX_AOT_SCAN LATX_TU_SCAN
unset LATX_DUMP LATX_SHOW_TB LAT_LOG LAT_LOG_FILENAME
cd "$sdk"
sha256sum "$latx" ./aikit_test ./libs/*.so >"$result/provenance.txt"
env | LC_ALL=C sort | grep -E '^(HOME=|LATX_|LBT_X86=|LD_LIBRARY_PATH=|AIKIT_)' >>"$result/provenance.txt"
TIMEFORMAT='real=%3R user=%3U sys=%3S'
for ((i=1; i<=count; i++)); do
    [[ ! -e output.pcm ]] || mv output.pcm "$result/previous-output-$i.pcm"
    { time timeout -k 2 35 "$latx" -L "$runtime" ./aikit_test \
        >"$result/$i.out" 2>"$result/$i.err"; } 2>"$result/$i.time"
    printf 'd3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac  output.pcm\n' | sha256sum -c -
    mv output.pcm "$result/$i.pcm"
    printf '%d %s\n' "$i" "$(<"$result/$i.time")" | tee -a "$result/times.txt"
    start=$SECONDS
    while :; do
        running=0
        for p in /proc/[0-9]*; do
            [[ $(readlink "$p/exe" 2>/dev/null) == "$latx" && -r $p/environ ]] || continue
            while IFS= read -r -d '' entry; do
                if [[ $entry == "HOME=$HOME" ]]; then running=1; break; fi
            done <"$p/environ" 2>/dev/null
        done
        (( running )) || break
        (( SECONDS-start < 90 )) || exit 1
        sleep .2
    done
done
awk '{split($2,a,"=");n++;s+=a[2];v[n]=a[2]}
    END{for(i=1;i<=n;i++)for(j=i+1;j<=n;j++)if(v[j]<v[i]){
        t=v[i];v[i]=v[j];v[j]=t}
    median=n%2?v[(n+1)/2]:(v[n/2]+v[n/2+1])/2;
    printf "n=%d mean=%.4f median=%.4f min=%.3f max=%.3f\n",n,s/n,median,v[1],v[n]}' \
    "$result/times.txt" | tee "$result/summary.txt"
