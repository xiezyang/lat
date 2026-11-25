#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# This fixture requires LSX writes to clear bits 255:128 in hardware.
set -eu
if [ "${LATX_LSX_AUTO_CLEAR:-0}" != 1 ]; then
    echo "SKIP: requires LATX_LSX_AUTO_CLEAR=1 and matching hardware"
    exit 77
fi
LATX_SOFTFPU=0 LATX_SOFTFPU_FAST=0 \
    exec python3 "$(dirname "$0")/test-avx-regressions.py" "$@"
