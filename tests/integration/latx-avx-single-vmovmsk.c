/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    PS_XMM_CASES = 16,
    PS_YMM_CASES = 256,
    PD_XMM_CASES = 4,
    PD_YMM_CASES = 16,
    RECORD_SIZE = 16,
    RECORDS = 2 * (PS_XMM_CASES + PS_YMM_CASES + PD_XMM_CASES +
                   PD_YMM_CASES),
    OUTPUT_SIZE = RECORDS * RECORD_SIZE,
};

static uint8_t output[OUTPUT_SIZE];
static uint32_t ps_xmm[PS_XMM_CASES][4];
static uint32_t ps_ymm[PS_YMM_CASES][8];
static uint64_t pd_xmm[PD_XMM_CASES][2];
static uint64_t pd_ymm[PD_YMM_CASES][4];

extern void latx_avx_single_vmovmsk_run(uint8_t *, const uint32_t *,
                                        const uint32_t *, const uint64_t *,
                                        const uint64_t *);

static void fill_inputs(void)
{
    for (unsigned mask = 0; mask < PS_XMM_CASES; ++mask) {
        for (unsigned lane = 0; lane < 4; ++lane) {
            uint32_t value = lane == 0
                                 ? 0
                                 : (lane & 1 ? UINT32_C(0x7f800001)
                                             : UINT32_C(0x7fc00001));
            ps_xmm[mask][lane] = value |
                                  ((mask >> lane) & 1 ? UINT32_C(0x80000000)
                                                      : 0);
        }
    }
    for (unsigned mask = 0; mask < PS_YMM_CASES; ++mask) {
        for (unsigned lane = 0; lane < 8; ++lane) {
            uint32_t value = lane == 0
                                 ? 0
                                 : (lane & 1 ? UINT32_C(0x7f800042)
                                             : UINT32_C(0x7fc00042));
            ps_ymm[mask][lane] = value |
                                  ((mask >> lane) & 1 ? UINT32_C(0x80000000)
                                                      : 0);
        }
    }
    for (unsigned mask = 0; mask < PD_XMM_CASES; ++mask) {
        for (unsigned lane = 0; lane < 2; ++lane) {
            uint64_t value = lane == 0
                                 ? 0
                                 : (lane & 1 ? UINT64_C(0x7ff0000000000001)
                                             : UINT64_C(0x7ff8000000000001));
            pd_xmm[mask][lane] = value |
                                 ((mask >> lane) & 1
                                      ? UINT64_C(0x8000000000000000)
                                      : 0);
        }
    }
    for (unsigned mask = 0; mask < PD_YMM_CASES; ++mask) {
        for (unsigned lane = 0; lane < 4; ++lane) {
            uint64_t value = lane == 0
                                 ? 0
                                 : (lane & 1 ? UINT64_C(0x7ff0000000000042)
                                             : UINT64_C(0x7ff8000000000042));
            pd_ymm[mask][lane] = value |
                                 ((mask >> lane) & 1
                                      ? UINT64_C(0x8000000000000000)
                                      : 0);
        }
    }
}

int latx_avx_single_main(long argc, char **argv)
{
    (void)argc;
    (void)argv;
    fill_inputs();
    latx_avx_single_vmovmsk_run(output, &ps_xmm[0][0], &ps_ymm[0][0],
                                 &pd_xmm[0][0], &pd_ymm[0][0]);
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}
