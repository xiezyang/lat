/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-random-common.h"

void latx_avx_random_vzeroupper(const uint8_t *, const uint8_t *, uint8_t *,
                                uint8_t *, uintptr_t);

int main(int argc, char **argv)
{
    return latx_avx_random_main("vzeroupper", latx_avx_random_vzeroupper,
                                argc, argv);
}
