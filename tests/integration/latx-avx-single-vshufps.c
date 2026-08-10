/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { OUTPUT_SIZE = 12 * 32 };
static uint8_t output[OUTPUT_SIZE];
extern void latx_avx_single_vshufps_run(uint8_t *);

int latx_avx_single_main(long argc, char **argv)
{
    (void)argc;
    (void)argv;
    latx_avx_single_vshufps_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}
