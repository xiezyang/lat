#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../target/i386/latx/include/xtts-matrix-native.h"

typedef void (*matrix_fn)(int, int, int, const int16_t *, const int16_t *,
                          const void *, int32_t *, int, int, int, int);
typedef void (*matrix8_fn)(int, int, const int16_t *, const int8_t *,
                           const int16_t *, int16_t *, int, int, int, int);
typedef void (*matrix_i8_fn)(int, int, const int8_t *, const int8_t *,
                             const int16_t *, int16_t *, int, int, int, int);
typedef void (*matrix16_fn)(int, int, int, const int16_t *, const int16_t *,
                            const int16_t *, int16_t *, int, int, int, int);
typedef void (*colmajor_fn)(int, int, int, const int16_t *, const int16_t *,
                            const int16_t *, int16_t *, int32_t *, int, int, int);
typedef void (*matrix_i8_w16_fn)(int, int, int, const int8_t *, const int16_t *,
                                const int16_t *, int16_t *, int, int, int, int);

int main(int argc, char **argv)
{
    void *lib = argc == 2 ? dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL) : NULL;
    if (!lib) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }
    matrix_fn funcs[2] = {
        dlsym(lib, "ivMatrixCalcMul_Add_Bias16_In16_W16_Out32"),
        dlsym(lib, "ivMatrixCalcMul_Add_Bias32_In16_W16_Out32")
    };
    matrix8_fn func8 = dlsym(lib, "ivMatrixCalc_AddBias16_In16_W8_Out16");
    matrix_i8_fn func_i8 = dlsym(lib, "ivMatrixCalc_AddBias16_In8_W8_Out16");
    matrix16_fn func16 = dlsym(lib, "ivMatrixCalcMul_AddBias16_In16_W16_Out16");
    colmajor_fn col = dlsym(lib, "ivMatrixAdd_AddBias16_In16_W16_Out16_ColMajor");
    matrix_i8_w16_fn i8_w16 = dlsym(lib, "ivMatrixCalc_AddBias16_In8_W16_Out16");
    if (!funcs[0] || !funcs[1] || !func8 || !func_i8 || !func16 || !col || !i8_w16) {
        return 2;
    }
    unsigned state = 12345;
    for (int test = 0; test < 10000; test++) {
        int columns = (rand_r(&state) % 32 + 1) * 8;
        int rows = (rand_r(&state) % 2 + 1) * 8;
        int16_t input[256], weights[256 * 16], bias16[16];
        int8_t weights8[256 * 16];
        int32_t bias32[16], actual[16], expected[16];
        int16_t actual8[16], expected8[16];
        for (int i = 0; i < columns; i++) input[i] = rand_r(&state);
        for (int i = 0; i < columns * rows; i++) {
            weights[i] = rand_r(&state);
            weights8[i] = rand_r(&state);
        }
        for (int i = 0; i < rows; i++) {
            bias16[i] = rand_r(&state);
            bias32[i] = rand_r(&state);
        }
        int iq = rand_r(&state) % 16, wq = rand_r(&state) % 16;
        int bq = rand_r(&state) % 24, oq = rand_r(&state) % 24;
        int multiplier = (int)(rand_r(&state) % 7) - 3;
        i8_w16(columns, rows, multiplier, (const int8_t *)input, weights,
                 bias16, actual8, iq, wq, bq, oq);
        xtts_matrix_i8_w16(columns, rows, multiplier, (const int8_t *)input,
                           weights, bias16, expected8, iq, wq, bq, oq);
        if (memcmp(actual8, expected8, rows * 2)) {
            fprintf(stderr, "matrix_i8_w16 mismatch test=%d\n", test);
            return 8;
        }
        for (int kind = 0; kind < 2; kind++) {
            const void *bias = kind ? (void *)bias32 : (void *)bias16;
            funcs[kind](columns, rows, 0, input, weights, bias, actual,
                         iq, wq, bq, oq);
            xtts_matrix_i16(columns, rows, input, weights, bias, expected,
                              iq, wq, bq, oq, kind);
            if (memcmp(actual, expected, rows * 4)) {
                fprintf(stderr, "matrix16 mismatch test=%d kind=%d q=%d,%d,%d,%d\n",
                        test, kind, iq, wq, bq, oq);
                return 3;
            }
        }
        func8(columns, rows, input, weights8, bias16, actual8, iq, wq, bq, oq);
        xtts_matrix_i16_w8(columns, rows, input, weights8, bias16, expected8,
                            iq, wq, bq, oq);
        if (memcmp(actual8, expected8, rows * 2)) {
            fprintf(stderr, "matrix8 mismatch test=%d\n", test);
            return 4;
        }
        func_i8(columns, rows, (const int8_t *)input, weights8, bias16,
                actual8, iq, wq, bq, oq);
        xtts_matrix_i8(columns, rows, (const int8_t *)input, weights8,
                       bias16, expected8, iq, wq, bq, oq);
        if (memcmp(actual8, expected8, rows * 2)) {
            fprintf(stderr, "matrix_i8 mismatch test=%d\n", test);
            return 5;
        }
        func16(columns, rows, 0, input, weights, bias16, actual8,
                 iq, wq, bq, oq);
        xtts_matrix_i16_out16(columns, rows, input, weights, bias16, expected8,
                               iq, wq, bq, oq);
        if (memcmp(actual8, expected8, rows * 2)) {
            fprintf(stderr, "matrix_out16 mismatch test=%d\n", test);
            return 6;
        }
        int16_t indices[16], colbias[256], cola[256], colb[256];
        int32_t scratcha[256], scratchb[256];
        for (int i = 0; i < rows; i++) indices[i] = rand_r(&state) % 16;
        for (int i = 0; i < columns; i++) colbias[i] = rand_r(&state);
        col(columns, rows, 0, indices, weights, colbias, cola, scratcha,
              iq, wq, bq);
        xtts_matrix_colmajor(columns, rows, indices, weights, colbias, colb,
                              scratchb, iq, wq, bq);
        if (memcmp(cola, colb, columns * 2) ||
            memcmp(scratcha, scratchb, columns * 4)) {
            fprintf(stderr, "colmajor mismatch test=%d columns=%d\n", test, columns);
            return 7;
        }
    }
    puts("PASS: 70000 matrix differential cases");
    return 0;
}
