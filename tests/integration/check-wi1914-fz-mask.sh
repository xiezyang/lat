#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
source="$root/target/i386/latx/translator/tr-avx.c"
block=$(sed -n '/^static void lsx_fp_apply_fz/,/^static void lsx_fp_status_finish/p' "$source")

if [[ "$block" != *'la_andi(field, mxcsr, 0x8000);'* ]]; then
    echo "missing MXCSR.FZ check" >&2
    exit 1
fi
if [[ "$block" != *'la_andi(field, mxcsr, 0x800);'* ]]; then
    echo "missing MXCSR.UM check" >&2
    exit 1
fi
if [[ "$block" != *'la_andi(field, mxcsr, 0x8000);'*'la_beq(field, zero_ir2_opnd, done);'*'la_andi(field, mxcsr, 0x800);'*'la_beq(field, zero_ir2_opnd, done);'* ]]; then
    echo "FZ/UM mask gates are not ordered before flush" >&2
    exit 1
fi
if [[ "$block" != *'la_ori(flags, flags, 0x30);'* ]]; then
    echo "masked-underflow flag update missing" >&2
    exit 1
fi

echo "PASS WI-1914 FZ/UM mask gate: FZ=1, UM=1 flushes; FZ=1, UM=0 bypasses flush"
