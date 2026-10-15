/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include <stdlib.h>
#ifndef MATRIX_HEADER
#define MATRIX_HEADER "../../target/i386/latx/include/xtts-matrix-native.h"
#endif
#include MATRIX_HEADER

/* Compare captures from the baseline and candidate native headers on LA. */
int main(void)
{
    unsigned seed = 26342;
    for (int test = 0; test < 1000; test++) {
        int columns = (rand_r(&seed) % 16 + 1) * 16;
        int rows = rand_r(&seed) % 16 + 1;
        int32_t storage[4096] __attribute__((aligned(16)));
        int16_t indices[16], bias[256], output[256];
        for (int i = 0; i < 4096; i++) storage[i] = rand_r(&seed);
        for (int i = 0; i < rows; i++) indices[i] = rand_r(&seed) % 16;
        for (int i = 0; i < columns; i++) bias[i] = rand_r(&seed);
        xtts_matrix_colmajor(columns, rows, indices, (int16_t *)storage,
                              bias, output, storage, 4, 6, 8);
        if (fwrite(storage, sizeof(storage), 1, stdout) != 1 ||
            fwrite(output, columns * sizeof(*output), 1, stdout) != 1) return 1;
    }
    return 0;
}
