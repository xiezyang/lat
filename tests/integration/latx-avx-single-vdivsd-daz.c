/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { RECORD_SIZE = 64 };

extern void latx_avx_single_vdivsd_daz_run(uint8_t *);

int latx_avx_single_main(long argc, char **argv)
{
    static uint8_t output[RECORD_SIZE];

    (void)argv;
    if (argc != 1) {
        return 72;
    }
    latx_avx_single_vdivsd_daz_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}
