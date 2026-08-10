/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { OUTPUT_SIZE = 2112 };
static uint8_t output[OUTPUT_SIZE];

extern void latx_avx_single_vzeroall_run(uint8_t *, uint8_t *);

int latx_avx_single_main(long argc, char **argv)
{
    (void)argv;
    if (argc != 1)
        return 2;
    latx_avx_single_vzeroall_run(output, 0);
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}
