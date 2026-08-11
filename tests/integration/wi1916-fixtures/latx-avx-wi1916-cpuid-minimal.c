/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

static uint8_t output[16];
extern void latx_avx_wi1916_cpuid_minimal_run(uint8_t *);

int latx_avx_single_main(long argc, char **argv)
{
    (void)argc;
    (void)argv;
    latx_avx_wi1916_cpuid_minimal_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}
