/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#ifndef MATRIX_HEADER
#define MATRIX_HEADER "../../target/i386/latx/include/xtts-matrix-native.h"
#endif
#include MATRIX_HEADER

int main(void)
{
    int16_t x[2048], w[2048 * 4];
    uint32_t sums[4], hash = 0;
    struct timespec begin, end;
    for (int i = 0; i < 2048; i++) x[i] = i * 71;
    for (int i = 0; i < 2048 * 4; i++) w[i] = i * 97;
    for (int count = 64; count <= 2048; count *= 2) {
        clock_gettime(CLOCK_MONOTONIC, &begin);
        for (int i = 0; i < 100000; i++) {
            xtts_dot_i16_four(x, w, 2048, count, sums);
            hash += sums[i & 3];
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("count=%d seconds=%.6f hash=%u\n", count,
               end.tv_sec - begin.tv_sec +
               (end.tv_nsec - begin.tv_nsec) * 1e-9, hash);
    }
    return 0;
}
