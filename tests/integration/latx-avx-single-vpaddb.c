/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { VPADDB_OUTPUT_SIZE = 32 };

extern void latx_avx_single_vpaddb_run(uint8_t *output);

static uint8_t vpaddb_output[VPADDB_OUTPUT_SIZE];

int latx_avx_single_main(long argc, char **argv)
{
    (void)argc;
    (void)argv;
    latx_avx_single_vpaddb_run(vpaddb_output);
    return latx_avx_single_write_all(vpaddb_output, sizeof(vpaddb_output)) != 0;
}
