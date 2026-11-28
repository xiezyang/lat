#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
if [ "${LATX_FLD_CLEAR:-0}" != 1 ]; then
    echo "SKIP: requires LATX_FLD_CLEAR=1 and matching scalar-load hardware"
    exit 77
fi
LATX_SOFTFPU=0 LATX_SOFTFPU_FAST=0 \
    exec python3 "$(dirname "$0")/test-avx-regressions.py" "$@"
