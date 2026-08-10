/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { VPXOR_OUTPUT_SIZE = 32 };

extern void latx_avx_single_vpxor_run(uint8_t *output);

static uint8_t vpxor_output[VPXOR_OUTPUT_SIZE];

int latx_avx_single_main(long argc, char **argv)
{
    (void)argc;
    (void)argv;
    latx_avx_single_vpxor_run(vpxor_output);
    return latx_avx_single_write_all(vpxor_output, sizeof(vpxor_output)) != 0;
}
