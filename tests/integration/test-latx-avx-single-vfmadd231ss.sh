#!/bin/sh
set -eu
exec "$(dirname "$0")/build-latx-avx-fma-xzy86.sh" vfmadd231ss "$@"
