/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include <stdlib.h>
#include "../../target/i386/latx/include/xtts-matrix-native.h"

#undef LATX_XTTS_MATRIX_NATIVE_H
#define XTTS_MATRIX_SCALAR
#define xtts_matrix_scale scalar_scale
#define xtts_matrix_convert scalar_convert
#define xtts_matrix_convert_four scalar_convert_four
#define xtts_float_to_i32 scalar_float_to_i32
#define xtts_matrix_colmajor scalar_colmajor
#define xtts_dot_i16_pair scalar_dot_i16_pair
#define xtts_dot_i16_four scalar_dot_i16_four
#define xtts_dot_i8_four scalar_dot_i8_four
#define xtts_matrix_i16 scalar_i16
#define xtts_matrix_i8 scalar_i8
#define xtts_matrix_i16_w8 scalar_i16_w8
#define xtts_matrix_i16_out16 scalar_i16_out16
#define xtts_matrix_i8_w16 scalar_i8_w16
#include "../../target/i386/latx/include/xtts-matrix-native.h"

#undef xtts_matrix_scale
#undef xtts_matrix_convert
#undef xtts_matrix_convert_four
#undef xtts_float_to_i32
#undef xtts_matrix_colmajor
#undef xtts_dot_i16_pair
#undef xtts_dot_i16_four
#undef xtts_dot_i8_four
#undef xtts_matrix_i16
#undef xtts_matrix_i8
#undef xtts_matrix_i16_w8
#undef xtts_matrix_i16_out16
#undef xtts_matrix_i8_w16

int main(void)
{
    static const uint32_t fp_bits[] = {
        0, 0x80000000, 1, 0x80000001, 0x00800000, 0x3f000000,
        0x3f800000, 0x40000000, 0x4f000000, 0x7f7fffff,
        0x7f800000, 0xff800000, 0x7fc00000,
    };
    const uint32_t extremes[4] = {0, 0x7fffffff, 0x80000000, 0xffffffff};
    const int32_t offsets[4] = {INT32_MIN, INT32_MAX, -1, 0};
    for (unsigned i = 0; i < sizeof(fp_bits) / sizeof(fp_bits[0]); i++) {
        for (unsigned j = 0; j < sizeof(fp_bits) / sizeof(fp_bits[0]); j++) {
            float scale, output_scale;
            int32_t converted[4], expected[4];
            memcpy(&scale, &fp_bits[i], sizeof(scale));
            memcpy(&output_scale, &fp_bits[j], sizeof(output_scale));
            xtts_matrix_convert_four(extremes, offsets, scale, output_scale,
                                       converted);
            scalar_convert_four(extremes, offsets, scale, output_scale, expected);
            if (memcmp(converted, expected, sizeof(converted))) return 9;
        }
    }
    unsigned state = 98765;
    for (int test = 0; test < 1000; test++) {
        int8_t input[4096], weights[4096 * 4];
        int columns = (rand_r(&state) % 512 + 1) * 8;
        for (int i = 0; i < columns; i++) input[i] = rand_r(&state);
        for (int i = 0; i < columns * 4; i++) weights[i] = rand_r(&state);
        uint32_t actual[4], expected[4];
        xtts_dot_i8_four(input, weights, columns, columns / 8, actual);
        scalar_dot_i8_four(input, weights, columns, columns / 8, expected);
        if (memcmp(actual, expected, sizeof(actual))) return 11;
    }
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
        uint32_t dots[4], scalar_dots[4];
        int dot_count = (rand_r(&state) % (columns / 4 + 1)) * 4;
        xtts_dot_i16_four(input, weights, columns, dot_count, dots);
        scalar_dot_i16_four(input, weights, columns, dot_count, scalar_dots);
        if (memcmp(dots, scalar_dots, sizeof(dots))) return 7;
        int groups = rand_r(&state) % (columns / 8 + 1);
        xtts_dot_i8_four((const int8_t *)input, weights8, columns, groups, dots);
        scalar_dot_i8_four((const int8_t *)input, weights8, columns, groups,
                            scalar_dots);
        if (memcmp(dots, scalar_dots, sizeof(dots))) return 10;
        for (int kind = 0; kind < 2; kind++) {
            const void *bias = kind ? (void *)bias32 : (void *)bias16;
            xtts_matrix_i16(columns, 16, input, weights, bias, actual,
                             iq, wq, bq, oq, kind);
            scalar_i16(columns, 16, input, weights, bias, expected,
                         iq, wq, bq, oq, kind);
            if (memcmp(actual, expected, sizeof(actual))) return 1;
            int rows = (rand_r(&state) % 8 + 1) * 2;
            xtts_matrix_i16(columns, rows, input, weights, bias, actual,
                             iq, wq, bq, oq, kind);
            scalar_i16(columns, rows, input, weights, bias, expected,
                         iq, wq, bq, oq, kind);
            if (memcmp(actual, expected, rows * sizeof(*actual))) return 8;
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
    puts("PASS: 111000 vector/scalar matrix and dot-product comparisons");
    return 0;
}
