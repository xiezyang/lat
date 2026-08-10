/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef LATX_VINTCMP_PREFIX
#error "LATX_VINTCMP_PREFIX must be defined by the fixture wrapper"
#endif
#ifndef LATX_VINTCMP_SYMBOL_PREFIX
#error "LATX_VINTCMP_SYMBOL_PREFIX must be defined by the fixture wrapper"
#endif

#include "latx-avx-single-common.h"

#define LATX_VINTCMP_JOIN_INNER(a, b) a##b
#define LATX_VINTCMP_JOIN(a, b) LATX_VINTCMP_JOIN_INNER(a, b)
#define LATX_VINTCMP_JOIN3_INNER(a, b, c) a##b##c
#define LATX_VINTCMP_JOIN3(a, b, c) LATX_VINTCMP_JOIN3_INNER(a, b, c)
#define LATX_VINTCMP_RUN \
    LATX_VINTCMP_JOIN3(latx_avx_single_, LATX_VINTCMP_PREFIX, _run)

enum {
    LATX_VINTCMP_CASES = 10,
    LATX_VINTCMP_OUTPUT_SIZE = LATX_VINTCMP_CASES * 32,
    LATX_VINTCMP_PAGE_SIZE = 4096,
};

extern void LATX_VINTCMP_RUN(uint8_t *, uint8_t *, int);

static uint8_t latx_vintcmp_output[LATX_VINTCMP_OUTPUT_SIZE];
static uint8_t latx_vintcmp_page[LATX_VINTCMP_PAGE_SIZE];

int latx_avx_single_main(long argc, char **argv)
{
    (void)argc;
    (void)argv;
    latx_avx_single_fill(latx_vintcmp_page,
                         sizeof(latx_vintcmp_page),
                         UINT64_C(0x24b7619d83e50acf));
    LATX_VINTCMP_RUN(latx_vintcmp_output, latx_vintcmp_page, 0);
    return latx_avx_single_write_all(latx_vintcmp_output,
                                     sizeof(latx_vintcmp_output)) != 0;
}
