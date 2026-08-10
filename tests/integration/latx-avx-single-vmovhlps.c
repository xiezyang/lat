/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { LATX_VMOVHLPS_OUTPUT_SIZE = 4 * 32 };

const uint64_t latx_vmovhlps_source_a[4] = {
    UINT64_C(0x0123456789abcdef), UINT64_C(0x8000000000000000),
    UINT64_C(0x7ff0000000000001), UINT64_C(0x8000000000000000),
};

const uint64_t latx_vmovhlps_source_b[4] = {
    UINT64_C(0xfedcba9876543210), UINT64_C(0x0000000000000000),
    UINT64_C(0x7ff0000000000042), UINT64_C(0x0000000000000000),
};

const uint64_t latx_vmovhlps_source_a_nan[4] = {
    UINT64_C(0x0123456789abcdef), UINT64_C(0x7ff8000000000042),
    UINT64_C(0x0123456789abcdef), UINT64_C(0x7ff8000000000042),
};

const uint64_t latx_vmovhlps_source_b_nan[4] = {
    UINT64_C(0xfedcba9876543210), UINT64_C(0x7ff0000000000042),
    UINT64_C(0xfedcba9876543210), UINT64_C(0x7ff0000000000042),
};

const uint64_t latx_vmovhlps_seed[4] = {
    UINT64_C(0xaaaaaaaaaaaaaaaa), UINT64_C(0xbbbbbbbbbbbbbbbb),
    UINT64_C(0xcccccccccccccccc), UINT64_C(0xdddddddddddddddd),
};

extern void latx_avx_single_vmovhlps_run(uint8_t *output);

static uint8_t output[LATX_VMOVHLPS_OUTPUT_SIZE];

int latx_avx_single_main(long argc, char **argv)
{
    (void)argc;
    (void)argv;
    latx_avx_single_vmovhlps_run(output);
    return latx_avx_single_write_all(output, sizeof(output)) == 0 ? 0 : 1;
}
