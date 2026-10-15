/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include <stdlib.h>
#include "../../target/i386/latx/include/xtts-matrix-native.h"

#undef LATX_XTTS_MATRIX_NATIVE_H
#define XTTS_MATRIX_SCALAR
#define xtts_matrix_scale scalar_scale
#define xtts_matrix_convert scalar_convert
#define xtts_float_to_i32 scalar_float_to_i32
#define xtts_matrix_colmajor scalar_colmajor
#define xtts_dot_i16_pair scalar_dot_i16_pair
#define xtts_matrix_i16 scalar_i16
#define xtts_matrix_i8 scalar_i8
#define xtts_matrix_i16_w8 scalar_i16_w8
#define xtts_matrix_i16_out16 scalar_i16_out16
#define xtts_matrix_i8_w16 scalar_i8_w16
#include "../../target/i386/latx/include/xtts-matrix-native.h"

#undef xtts_matrix_scale
#undef xtts_matrix_convert
#undef xtts_float_to_i32
#undef xtts_matrix_colmajor
#undef xtts_dot_i16_pair
#undef xtts_matrix_i16
#undef xtts_matrix_i8
#undef xtts_matrix_i16_w8
#undef xtts_matrix_i16_out16
#undef xtts_matrix_i8_w16

int main(void)
{
    unsigned state = 98765;
    for (int test = 0; test < 10000; test++) {
        int columns = (rand_r(&state) % 64 + 1) * 4;
        int16_t input[256], weights[256 * 16], bias16[16];
        int8_t weights8[256 * 16];
        int32_t bias32[16], actual[16], expected[16];
        int16_t actual8[16], expected8[16];
        for (int i = 0; i < columns; i++) input[i] = rand_r(&state);
        for (int i = 0; i < columns * 16; i++) {
            weights[i] = rand_r(&state);
            weights8[i] = rand_r(&state);
        }
        for (int i = 0; i < 16; i++) {
            bias16[i] = rand_r(&state);
            bias32[i] = rand_r(&state);
        }
        int iq = rand_r(&state) % 16, wq = rand_r(&state) % 16;
        int bq = rand_r(&state) % 24, oq = rand_r(&state) % 24;
        for (int kind = 0; kind < 2; kind++) {
            const void *bias = kind ? (void *)bias32 : (void *)bias16;
            xtts_matrix_i16(columns, 16, input, weights, bias, actual,
                             iq, wq, bq, oq, kind);
            scalar_i16(columns, 16, input, weights, bias, expected,
                         iq, wq, bq, oq, kind);
            if (memcmp(actual, expected, sizeof(actual))) return 1;
        }
        xtts_matrix_i8(columns, 16, (const int8_t *)input, weights8, bias16,
                        actual8, iq, wq, bq, oq);
        scalar_i8(columns, 16, (const int8_t *)input, weights8, bias16,
                    expected8, iq, wq, bq, oq);
        if (memcmp(actual8, expected8, sizeof(actual8))) return 2;
        xtts_matrix_i16_w8(columns, 16, input, weights8, bias16,
                            actual8, iq, wq, bq, oq);
        scalar_i16_w8(columns, 16, input, weights8, bias16,
                        expected8, iq, wq, bq, oq);
        if (memcmp(actual8, expected8, sizeof(actual8))) return 3;
        xtts_matrix_i16_out16(columns, 16, input, weights, bias16,
                               actual8, iq, wq, bq, oq);
        scalar_i16_out16(columns, 16, input, weights, bias16,
                          expected8, iq, wq, bq, oq);
        if (memcmp(actual8, expected8, sizeof(actual8))) return 4;
        xtts_matrix_i8_w16(columns, 16, 3, (const int8_t *)input, weights,
                            bias16, actual8, iq, wq, bq, oq);
        scalar_i8_w16(columns, 16, 3, (const int8_t *)input, weights,
                       bias16, expected8, iq, wq, bq, oq);
        if (memcmp(actual8, expected8, sizeof(actual8))) return 6;
        int16_t indices[16], colbias[256], cola[256], colb[256];
        int32_t sa[256] __attribute__((aligned(16)));
        int32_t sb[256] __attribute__((aligned(16)));
        for (int i = 0; i < 16; i++) indices[i] = rand_r(&state) % 16;
        for (int i = 0; i < columns; i++) colbias[i] = rand_r(&state);
        xtts_matrix_colmajor(columns, 16, indices, weights, colbias, cola, sa,
                              iq, wq, bq);
        scalar_colmajor(columns, 16, indices, weights, colbias, colb, sb,
                          iq, wq, bq);
        if (memcmp(cola, colb, columns * 2) || memcmp(sa, sb, columns * 4)) {
            return 5;
        }
    }
    puts("PASS: 70000 vector/scalar matrix comparisons");
    return 0;
}
