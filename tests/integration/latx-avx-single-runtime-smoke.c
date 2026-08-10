/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

int latx_avx_single_main(long argc, char **argv)
{
    uint8_t output[32];

    (void)argc;
    (void)argv;
    latx_avx_single_fill(output, sizeof(output),
                         UINT64_C(0x6a09e667f3bcc909));
    return latx_avx_single_write_all(output, sizeof(output)) == 0 ? 0 : 2;
}
