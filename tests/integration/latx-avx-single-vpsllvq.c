/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { LATX_AVX_SINGLE_VPSLLVQ_OUTPUT_SIZE = 256 };
extern void latx_avx_single_vpsllvq_run(uint8_t *output);

static uint8_t output[LATX_AVX_SINGLE_VPSLLVQ_OUTPUT_SIZE];

int latx_avx_single_main(long argc, char **argv)
{
    (void)argc;
    (void)argv;
    latx_avx_single_vpsllvq_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}
