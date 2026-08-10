#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 RAW_DIRECTORY" >&2
    exit 2
fi

root=$1
normal=$root/x86-vcvtsd2ss
daz=$root/x86-vcvtsd2ss-daz

check_status_size() {
    local status_file=$1 expected_status=$2 data_file=$3 expected_size=$4
    [[ "$(tr -d '[:space:]' < "$status_file")" == "$expected_status" ]]
    [[ "$(stat -c %s "$data_file")" == "$expected_size" ]]
}

check_status_size "$normal/reference.status" 0 "$normal/reference.bin" 1152
check_status_size "$daz/reference.status" 0 "$daz/reference.bin" 64

for stderr in "$normal"/*.stderr "$daz"/*.stderr; do
    [[ ! -s "$stderr" ]]
done

for item in \
    "fpe-invalid 7 00001f01" \
    "fpe-precision 6 00000fa0" \
    "fpe-overflow 4 00001ba8" \
    "fpe-underflow 5 00001792"; do
    read -r name si_code mxcsr <<< "$item"
    record=$normal/$name.bin
    check_status_size "$normal/$name.status" 136 "$record" 64
    read -r signal code < <(od -An -tu4 -j0 -N8 "$record")
    [[ "$signal" == 8 && "$code" == "$si_code" ]]
    actual_mxcsr=$(od -An -tx4 -j28 -N4 "$record" | tr -d '[:space:]')
    [[ "$actual_mxcsr" == "$mxcsr" ]]
    high=$(od -An -tx1 -j48 -N16 "$record" | tr -d '[:space:]')
    [[ "$high" == 887766554433221100ffeeddccbbaa99 ]]
done

echo "PASS WI-1875 xzy86 VCVTSD2SS records: masked and unmasked overflow/underflow verified"
