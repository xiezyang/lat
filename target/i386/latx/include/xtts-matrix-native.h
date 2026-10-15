/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LATX_XTTS_MATRIX_NATIVE_H
#define LATX_XTTS_MATRIX_NATIVE_H

#include <stdint.h>
#include <string.h>

static float xtts_matrix_scale(int exponent)
{
    int32_t factor = (int32_t)(UINT32_C(1) <<
                              ((exponent < 0 ? -exponent : exponent) & 31));
    return exponent < 0 ? 1.0f / (float)factor : (float)factor;
}

static int32_t xtts_float_to_i32(float value)
{
    if (!(value >= -2147483648.0f && value < 2147483648.0f)) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int32_t xtts_matrix_convert(uint32_t sum, int32_t bias,
                                   float scale, float output_scale)
{
    /* Separate rounding at each SSE scalar operation; never contract to FMA. */
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
    float value;
    float input = (float)(int32_t)sum;
    float offset = (float)bias;
    __asm__ (
        "fmul.s %[value], %[input], %[scale]\n\t"
        "fadd.s %[value], %[value], %[bias]\n\t"
        "fmul.s %[value], %[value], %[output_scale]\n\t"
        : [value] "=&f"(value)
        : [input] "f"(input), [scale] "f"(scale), [bias] "f"(offset),
          [output_scale] "f"(output_scale));
#else
    volatile float product = (float)(int32_t)sum * scale;
    volatile float biased = product + (float)bias;
    float value = biased * output_scale;
#endif
    return xtts_float_to_i32(value);
}

static void xtts_matrix_convert_four(const uint32_t sums[4],
                                      const int32_t biases[4], float scale,
                                      float output_scale, int32_t output[4])
{
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
    uint32_t scale_bits, output_scale_bits;
    memcpy(&scale_bits, &scale, sizeof(scale_bits));
    memcpy(&output_scale_bits, &output_scale, sizeof(output_scale_bits));
    __asm__ volatile (
        "vld $vr0, %[sum], 0\n\t"
        "vld $vr1, %[bias], 0\n\t"
        "vffint.s.w $vr0, $vr0\n\t"
        "vffint.s.w $vr1, $vr1\n\t"
        "vreplgr2vr.w $vr2, %[scale]\n\t"
        "vreplgr2vr.w $vr3, %[output_scale]\n\t"
        "vfmul.s $vr0, $vr0, $vr2\n\t"
        "vfadd.s $vr0, $vr0, $vr1\n\t"
        "vfmul.s $vr0, $vr0, $vr3\n\t"
        "vreplgr2vr.w $vr1, %[lower]\n\t"
        "vreplgr2vr.w $vr2, %[upper]\n\t"
        "vfcmp.cle.s $vr1, $vr1, $vr0\n\t"
        "vfcmp.clt.s $vr2, $vr0, $vr2\n\t"
        "vand.v $vr1, $vr1, $vr2\n\t"
        "vftintrz.w.s $vr0, $vr0\n\t"
        "vand.v $vr0, $vr0, $vr1\n\t"
        "vnor.v $vr1, $vr1, $vr1\n\t"
        "vreplgr2vr.w $vr2, %[invalid]\n\t"
        "vand.v $vr1, $vr1, $vr2\n\t"
        "vor.v $vr0, $vr0, $vr1\n\t"
        "vst $vr0, %[out], 0\n\t"
        :
        : [sum] "r"(sums), [bias] "r"(biases), [out] "r"(output),
          [scale] "r"(scale_bits), [output_scale] "r"(output_scale_bits),
          [lower] "r"(UINT32_C(0xcf000000)),
          [upper] "r"(UINT32_C(0x4f000000)),
          [invalid] "r"(INT32_MIN)
        : "f0", "f1", "f2", "f3", "memory");
#else
    for (int k = 0; k < 4; k++) {
        output[k] = xtts_matrix_convert(sums[k], biases[k], scale, output_scale);
    }
#endif
}

static void xtts_matrix_colmajor(int columns, int rows,
                                 const int16_t *indices,
                                 const int16_t *weights, const int16_t *bias,
                                 int16_t *output, int32_t *scratch,
                                 int input_q, int weight_q, int bias_q)
{
    float scale = xtts_matrix_scale(bias_q - input_q - weight_q);
    float inverse_scale = 1.0f / scale;
    int count = ((columns / 4) & ~3) * 4;

    for (int i = 0; i < columns; i++) {
        scratch[i] = xtts_float_to_i32((float)bias[i] * inverse_scale);
    }
    for (int row = 0; row < rows; row++) {
        const int16_t *w = weights + (int64_t)indices[row] * columns;
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
        if (count > 0) {
            int32_t *dst = scratch;
            int n = count;
            __asm__ volatile (
                "1:\n\t"
                "vldrepl.d $vr0, %[w], 0\n\t"
                "vsllwil.w.h $vr0, $vr0, 0\n\t"
                "vld $vr1, %[dst], 0\n\t"
                "vadd.w $vr0, $vr0, $vr1\n\t"
                "vst $vr0, %[dst], 0\n\t"
                "addi.d %[w], %[w], 8\n\t"
                "addi.d %[dst], %[dst], 16\n\t"
                "addi.w %[n], %[n], -4\n\t"
                "bnez %[n], 1b\n\t"
                : [w] "+&r"(w), [dst] "+&r"(dst), [n] "+&r"(n)
                : : "f0", "f1", "memory");
        }
#else
        for (int i = 0; i < count; i++) {
            scratch[i] = (uint32_t)scratch[i] + (uint32_t)(int32_t)w[i];
        }
#endif
    }
    for (int i = 0; i < columns; i++) {
        output[i] = (uint16_t)xtts_float_to_i32((float)scratch[i] * scale);
    }
}

static void xtts_dot_i16_pair(const int16_t *input, const int16_t *w0,
                               const int16_t *w1, int count,
                               uint32_t *sum0, uint32_t *sum1)
{
    uint32_t a = 0, b = 0;
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
    if (count > 0) {
        __asm__ volatile (
            "vxor.v $vr3, $vr3, $vr3\n\t"
            "vxor.v $vr4, $vr4, $vr4\n\t"
            "sltui %[a], %[n], 8\n\t"
            "bnez %[a], 2f\n\t"
            "1:\n\t"
            "vld $vr0, %[x], 0\n\t"
            "vld $vr1, %[w0], 0\n\t"
            "vld $vr2, %[w1], 0\n\t"
            "vmaddwev.w.h $vr3, $vr0, $vr1\n\t"
            "vmaddwod.w.h $vr3, $vr0, $vr1\n\t"
            "vmaddwev.w.h $vr4, $vr0, $vr2\n\t"
            "vmaddwod.w.h $vr4, $vr0, $vr2\n\t"
            "addi.d %[x], %[x], 16\n\t"
            "addi.d %[w0], %[w0], 16\n\t"
            "addi.d %[w1], %[w1], 16\n\t"
            "addi.w %[n], %[n], -8\n\t"
            "sltui %[a], %[n], 8\n\t"
            "beqz %[a], 1b\n\t"
            "2:\n\t"
            "beqz %[n], 3f\n\t"
            "vldrepl.d $vr0, %[x], 0\n\t"
            "vldrepl.d $vr1, %[w0], 0\n\t"
            "vldrepl.d $vr2, %[w1], 0\n\t"
            "vsllwil.w.h $vr0, $vr0, 0\n\t"
            "vsllwil.w.h $vr1, $vr1, 0\n\t"
            "vsllwil.w.h $vr2, $vr2, 0\n\t"
            "vmadd.w $vr3, $vr0, $vr1\n\t"
            "vmadd.w $vr4, $vr0, $vr2\n\t"
            "3:\n\t"
            "vshuf4i.w $vr0, $vr3, 0x4e\n\t"
            "vshuf4i.w $vr1, $vr4, 0x4e\n\t"
            "vadd.w $vr3, $vr3, $vr0\n\t"
            "vadd.w $vr4, $vr4, $vr1\n\t"
            "vshuf4i.w $vr0, $vr3, 0xb1\n\t"
            "vshuf4i.w $vr1, $vr4, 0xb1\n\t"
            "vadd.w $vr3, $vr3, $vr0\n\t"
            "vadd.w $vr4, $vr4, $vr1\n\t"
            "vpickve2gr.w %[a], $vr3, 0\n\t"
            "vpickve2gr.w %[b], $vr4, 0\n\t"
            : [x] "+&r"(input), [w0] "+&r"(w0), [w1] "+&r"(w1),
              [n] "+&r"(count), [a] "=&r"(a), [b] "=&r"(b)
            : : "f0", "f1", "f2", "f3", "f4", "memory");
    }
#else
    for (int i = 0; i < count; i++) {
        a += (uint32_t)((int32_t)input[i] * w0[i]);
        b += (uint32_t)((int32_t)input[i] * w1[i]);
    }
#endif
    *sum0 = a;
    *sum1 = b;
}

static void xtts_dot_i16_four(const int16_t *input, const int16_t *weights,
                              int stride, int count, uint32_t sums[4])
{
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
    const int16_t *w0 = weights, *w1 = w0 + stride;
    const int16_t *w2 = w1 + stride, *w3 = w2 + stride;
    unsigned tail;
    if (count <= 0) {
        memset(sums, 0, 4 * sizeof(*sums));
        return;
    }
    __asm__ volatile (
        "vxor.v $vr5, $vr5, $vr5\n\t"
        "vxor.v $vr6, $vr6, $vr6\n\t"
        "vxor.v $vr7, $vr7, $vr7\n\t"
        "vxor.v $vr8, $vr8, $vr8\n\t"
        "sltui %[tail], %[n], 8\n\t"
        "bnez %[tail], 2f\n\t"
        "1:\n\t"
        "vld $vr0, %[x], 0\n\t"
        "vld $vr1, %[w0], 0\n\t"
        "vld $vr2, %[w1], 0\n\t"
        "vld $vr3, %[w2], 0\n\t"
        "vld $vr4, %[w3], 0\n\t"
        "vmaddwev.w.h $vr5, $vr0, $vr1\n\t"
        "vmaddwod.w.h $vr5, $vr0, $vr1\n\t"
        "vmaddwev.w.h $vr6, $vr0, $vr2\n\t"
        "vmaddwod.w.h $vr6, $vr0, $vr2\n\t"
        "vmaddwev.w.h $vr7, $vr0, $vr3\n\t"
        "vmaddwod.w.h $vr7, $vr0, $vr3\n\t"
        "vmaddwev.w.h $vr8, $vr0, $vr4\n\t"
        "vmaddwod.w.h $vr8, $vr0, $vr4\n\t"
        "addi.d %[x], %[x], 16\n\t"
        "addi.d %[w0], %[w0], 16\n\t"
        "addi.d %[w1], %[w1], 16\n\t"
        "addi.d %[w2], %[w2], 16\n\t"
        "addi.d %[w3], %[w3], 16\n\t"
        "addi.w %[n], %[n], -8\n\t"
        "sltui %[tail], %[n], 8\n\t"
        "beqz %[tail], 1b\n\t"
        "2:\n\t"
        "beqz %[n], 3f\n\t"
        "vldrepl.d $vr0, %[x], 0\n\t"
        "vldrepl.d $vr1, %[w0], 0\n\t"
        "vldrepl.d $vr2, %[w1], 0\n\t"
        "vldrepl.d $vr3, %[w2], 0\n\t"
        "vldrepl.d $vr4, %[w3], 0\n\t"
        "vsllwil.w.h $vr0, $vr0, 0\n\t"
        "vsllwil.w.h $vr1, $vr1, 0\n\t"
        "vsllwil.w.h $vr2, $vr2, 0\n\t"
        "vsllwil.w.h $vr3, $vr3, 0\n\t"
        "vsllwil.w.h $vr4, $vr4, 0\n\t"
        "vmadd.w $vr5, $vr0, $vr1\n\t"
        "vmadd.w $vr6, $vr0, $vr2\n\t"
        "vmadd.w $vr7, $vr0, $vr3\n\t"
        "vmadd.w $vr8, $vr0, $vr4\n\t"
        "3:\n\t"
        "vshuf4i.w $vr0, $vr5, 0x4e\n\t"
        "vshuf4i.w $vr1, $vr6, 0x4e\n\t"
        "vshuf4i.w $vr2, $vr7, 0x4e\n\t"
        "vshuf4i.w $vr3, $vr8, 0x4e\n\t"
        "vadd.w $vr5, $vr5, $vr0\n\t"
        "vadd.w $vr6, $vr6, $vr1\n\t"
        "vadd.w $vr7, $vr7, $vr2\n\t"
        "vadd.w $vr8, $vr8, $vr3\n\t"
        "vshuf4i.w $vr0, $vr5, 0xb1\n\t"
        "vshuf4i.w $vr1, $vr6, 0xb1\n\t"
        "vshuf4i.w $vr2, $vr7, 0xb1\n\t"
        "vshuf4i.w $vr3, $vr8, 0xb1\n\t"
        "vadd.w $vr5, $vr5, $vr0\n\t"
        "vadd.w $vr6, $vr6, $vr1\n\t"
        "vadd.w $vr7, $vr7, $vr2\n\t"
        "vadd.w $vr8, $vr8, $vr3\n\t"
        "vstelm.w $vr5, %[out], 0, 0\n\t"
        "vstelm.w $vr6, %[out], 4, 0\n\t"
        "vstelm.w $vr7, %[out], 8, 0\n\t"
        "vstelm.w $vr8, %[out], 12, 0\n\t"
        : [x] "+&r"(input), [w0] "+&r"(w0), [w1] "+&r"(w1),
          [w2] "+&r"(w2), [w3] "+&r"(w3), [n] "+&r"(count),
          [tail] "=&r"(tail)
        : [out] "r"(sums)
        : "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
          "memory");
#else
    xtts_dot_i16_pair(input, weights, weights + stride, count,
                       &sums[0], &sums[1]);
    xtts_dot_i16_pair(input, weights + (int64_t)stride * 2,
                       weights + (int64_t)stride * 3, count,
                       &sums[2], &sums[3]);
#endif
}

static void xtts_matrix_i16(int columns, int rows, const int16_t *input,
                             const int16_t *weights, const void *bias,
                             int32_t *output, int input_q, int weight_q,
                             int bias_q, int output_q, int bias32)
{
    int count = (columns / 4) * 4;
    float scale = xtts_matrix_scale(bias_q - input_q - weight_q);
    float output_scale = xtts_matrix_scale(output_q - bias_q);

    int row = 0;
    for (; row + 3 < rows; row += 4) {
        uint32_t sums[4];
        int32_t biases[4];
        const int16_t *w = weights + (int64_t)row * columns;
        xtts_dot_i16_four(input, w, columns, count, sums);
        for (int k = 0; k < 4; k++) {
            biases[k] = bias32 ? ((const int32_t *)bias)[row + k] :
                                ((const int16_t *)bias)[row + k];
        }
        xtts_matrix_convert_four(sums, biases, scale, output_scale, output + row);
    }
    for (; row < rows; row += 2) {
        uint32_t sum0 = 0, sum1 = 0;
        const int16_t *w0 = weights + (int64_t)row * columns;
        const int16_t *w1 = w0 + columns;
        xtts_dot_i16_pair(input, w0, w1, count, &sum0, &sum1);
        int32_t b0 = bias32 ? ((const int32_t *)bias)[row] :
                             ((const int16_t *)bias)[row];
        int32_t b1 = bias32 ? ((const int32_t *)bias)[row + 1] :
                             ((const int16_t *)bias)[row + 1];
        output[row] = xtts_matrix_convert(sum0, b0, scale, output_scale);
        output[row + 1] = xtts_matrix_convert(sum1, b1, scale, output_scale);
    }
}

static void xtts_matrix_i16_out16(int columns, int rows,
                                  const int16_t *input,
                                  const int16_t *weights, const int16_t *bias,
                                  int16_t *output, int input_q, int weight_q,
                                  int bias_q, int output_q)
{
    int count = (columns / 4) * 4;
    float scale = xtts_matrix_scale(bias_q - input_q - weight_q);
    float output_scale = xtts_matrix_scale(output_q - bias_q);

    for (int row = 0; row < rows; row += 4) {
        uint32_t sums[4];
        int32_t biases[4], converted[4];
        const int16_t *w = weights + (int64_t)row * columns;
        xtts_dot_i16_four(input, w, columns, count, sums);
        for (int k = 0; k < 4; k++) {
            biases[k] = bias[row + k];
        }
        xtts_matrix_convert_four(sums, biases, scale, output_scale, converted);
        for (int k = 0; k < 4; k++) {
            output[row + k] = (uint16_t)converted[k];
        }
    }
}

static void xtts_matrix_i8_w16(int columns, int rows, int multiplier,
                               const int8_t *input, const int16_t *weights,
                               const int16_t *bias, int16_t *output,
                               int input_q, int weight_q, int bias_q,
                               int output_q)
{
    int count = (columns / 8) * 8;
    float scale = xtts_matrix_scale(bias_q - input_q - weight_q);
    float output_scale = xtts_matrix_scale(output_q - bias_q);

    for (int row = 0; row < rows; row += 2) {
        uint32_t sums[2] = {0, 0};
        const int16_t *w0 = weights + (int64_t)row * columns;
        const int16_t *w1 = w0 + columns;
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
        if (count > 0) {
            const int8_t *x = input;
            int n = count;
            __asm__ volatile (
                "vxor.v $vr3, $vr3, $vr3\n\t"
                "vxor.v $vr4, $vr4, $vr4\n\t"
                "1:\n\t"
                "vldrepl.d $vr0, %[x], 0\n\t"
                "vsllwil.h.b $vr0, $vr0, 0\n\t"
                "vld $vr1, %[w0], 0\n\t"
                "vld $vr2, %[w1], 0\n\t"
                "vmaddwev.w.h $vr3, $vr0, $vr1\n\t"
                "vmaddwod.w.h $vr3, $vr0, $vr1\n\t"
                "vmaddwev.w.h $vr4, $vr0, $vr2\n\t"
                "vmaddwod.w.h $vr4, $vr0, $vr2\n\t"
                "addi.d %[x], %[x], 8\n\t"
                "addi.d %[w0], %[w0], 16\n\t"
                "addi.d %[w1], %[w1], 16\n\t"
                "addi.w %[n], %[n], -8\n\t"
                "bnez %[n], 1b\n\t"
                "vshuf4i.w $vr0, $vr3, 0x4e\n\t"
                "vshuf4i.w $vr1, $vr4, 0x4e\n\t"
                "vadd.w $vr3, $vr3, $vr0\n\t"
                "vadd.w $vr4, $vr4, $vr1\n\t"
                "vshuf4i.w $vr0, $vr3, 0xb1\n\t"
                "vshuf4i.w $vr1, $vr4, 0xb1\n\t"
                "vadd.w $vr3, $vr3, $vr0\n\t"
                "vadd.w $vr4, $vr4, $vr1\n\t"
                "vpickve2gr.w %[a], $vr3, 0\n\t"
                "vpickve2gr.w %[b], $vr4, 0\n\t"
                : [x] "+&r"(x), [w0] "+&r"(w0), [w1] "+&r"(w1),
                  [n] "+&r"(n), [a] "=&r"(sums[0]), [b] "=&r"(sums[1])
                : : "f0", "f1", "f2", "f3", "f4", "memory");
        }
#else
        for (int i = 0; i < count; i++) {
            sums[0] += (uint32_t)((int32_t)input[i] * w0[i]);
            sums[1] += (uint32_t)((int32_t)input[i] * w1[i]);
        }
#endif
        for (int k = 0; k < 2; k++) {
            volatile float product = (float)(int32_t)sums[k] * scale;
            volatile float biased = product + (float)bias[row + k];
            volatile float scaled = biased * output_scale;
            int32_t value = xtts_float_to_i32(scaled * (float)multiplier);
            output[row + k] = value > INT16_MAX ? INT16_MAX :
                              value < INT16_MIN ? INT16_MIN : value;
        }
    }
}

static void xtts_dot_i8_four(const int8_t *input, const int8_t *weights,
                             int stride, int groups, uint32_t sums[4])
{
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
    if (groups <= 0) {
        memset(sums, 0, 4 * sizeof(*sums));
        return;
    }
    const int8_t *w0 = weights, *w1 = w0 + stride;
    const int8_t *w2 = w1 + stride, *w3 = w2 + stride;
    __asm__ volatile (
        "vxor.v $vr5, $vr5, $vr5\n\t"
        "vxor.v $vr6, $vr6, $vr6\n\t"
        "vxor.v $vr7, $vr7, $vr7\n\t"
        "vxor.v $vr8, $vr8, $vr8\n\t"
        "1:\n\t"
        "vldrepl.d $vr0, %[x], 0\n\t"
        "vldrepl.d $vr1, %[w0], 0\n\t"
        "vldrepl.d $vr2, %[w1], 0\n\t"
        "vldrepl.d $vr3, %[w2], 0\n\t"
        "vldrepl.d $vr4, %[w3], 0\n\t"
        "vsllwil.h.b $vr0, $vr0, 0\n\t"
        "vsllwil.h.b $vr1, $vr1, 0\n\t"
        "vsllwil.h.b $vr2, $vr2, 0\n\t"
        "vsllwil.h.b $vr3, $vr3, 0\n\t"
        "vsllwil.h.b $vr4, $vr4, 0\n\t"
        "vmadd.h $vr5, $vr0, $vr1\n\t"
        "vmadd.h $vr6, $vr0, $vr2\n\t"
        "vmadd.h $vr7, $vr0, $vr3\n\t"
        "vmadd.h $vr8, $vr0, $vr4\n\t"
        "addi.d %[x], %[x], 8\n\t"
        "addi.d %[w0], %[w0], 8\n\t"
        "addi.d %[w1], %[w1], 8\n\t"
        "addi.d %[w2], %[w2], 8\n\t"
        "addi.d %[w3], %[w3], 8\n\t"
        "addi.w %[n], %[n], -1\n\t"
        "bnez %[n], 1b\n\t"
        "vxor.v $vr0, $vr0, $vr0\n\t"
        "vaddwev.w.h $vr1, $vr5, $vr0\n\t"
        "vaddwod.w.h $vr5, $vr5, $vr0\n\t"
        "vadd.w $vr5, $vr5, $vr1\n\t"
        "vaddwev.w.h $vr1, $vr6, $vr0\n\t"
        "vaddwod.w.h $vr6, $vr6, $vr0\n\t"
        "vadd.w $vr6, $vr6, $vr1\n\t"
        "vaddwev.w.h $vr1, $vr7, $vr0\n\t"
        "vaddwod.w.h $vr7, $vr7, $vr0\n\t"
        "vadd.w $vr7, $vr7, $vr1\n\t"
        "vaddwev.w.h $vr1, $vr8, $vr0\n\t"
        "vaddwod.w.h $vr8, $vr8, $vr0\n\t"
        "vadd.w $vr8, $vr8, $vr1\n\t"
        "vshuf4i.w $vr0, $vr5, 0x4e\n\t"
        "vshuf4i.w $vr1, $vr6, 0x4e\n\t"
        "vshuf4i.w $vr2, $vr7, 0x4e\n\t"
        "vshuf4i.w $vr3, $vr8, 0x4e\n\t"
        "vadd.w $vr5, $vr5, $vr0\n\t"
        "vadd.w $vr6, $vr6, $vr1\n\t"
        "vadd.w $vr7, $vr7, $vr2\n\t"
        "vadd.w $vr8, $vr8, $vr3\n\t"
        "vshuf4i.w $vr0, $vr5, 0xb1\n\t"
        "vshuf4i.w $vr1, $vr6, 0xb1\n\t"
        "vshuf4i.w $vr2, $vr7, 0xb1\n\t"
        "vshuf4i.w $vr3, $vr8, 0xb1\n\t"
        "vadd.w $vr5, $vr5, $vr0\n\t"
        "vadd.w $vr6, $vr6, $vr1\n\t"
        "vadd.w $vr7, $vr7, $vr2\n\t"
        "vadd.w $vr8, $vr8, $vr3\n\t"
        "vstelm.w $vr5, %[out], 0, 0\n\t"
        "vstelm.w $vr6, %[out], 4, 0\n\t"
        "vstelm.w $vr7, %[out], 8, 0\n\t"
        "vstelm.w $vr8, %[out], 12, 0\n\t"
        : [x] "+&r"(input), [w0] "+&r"(w0), [w1] "+&r"(w1),
          [w2] "+&r"(w2), [w3] "+&r"(w3), [n] "+&r"(groups)
        : [out] "r"(sums)
        : "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
          "memory");
#else
    for (int row = 0; row < 4; row++) {
        uint16_t lanes[8] = {0};
        const int8_t *w = weights + (int64_t)row * stride;
        for (int g = 0; g < groups; g++) {
            for (int lane = 0; lane < 8; lane++) {
                int i = g * 8 + lane;
                lanes[lane] += (int16_t)input[i] * (int16_t)w[i];
            }
        }
        int32_t sum = 0;
        for (int lane = 0; lane < 8; lane++) {
            sum += (int16_t)lanes[lane];
        }
        sums[row] = sum;
    }
#endif
}

static void xtts_matrix_i8(int columns, int rows, const int8_t *input,
                            const int8_t *weights, const int16_t *bias,
                            int16_t *output, int input_q, int weight_q,
                            int bias_q, int output_q)
{
    int groups = (columns / 8) & ~3;
    float scale = xtts_matrix_scale(bias_q - input_q - weight_q);
    float output_scale = xtts_matrix_scale(output_q - bias_q);

    /* The original wraps each of its eight 16-bit accumulators separately. */
    for (int row = 0; row < rows; row += 8) {
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
        for (int k = 0; k < 8; k += 4) {
            uint32_t sums[4];
            int32_t biases[4], converted[4];
            xtts_dot_i8_four(input, weights + (int64_t)(row + k) * columns,
                              columns, groups, sums);
            for (int j = 0; j < 4; j++) {
                biases[j] = bias[row + k + j];
            }
            xtts_matrix_convert_four(sums, biases, scale, output_scale, converted);
            for (int j = 0; j < 4; j++) {
                int32_t value = converted[j];
                output[row + k + j] = value > INT16_MAX ? INT16_MAX :
                                      value < INT16_MIN ? INT16_MIN : value;
            }
        }
#else
        for (int k = 0; k < 8; k++) {
            const int8_t *w = weights + (int64_t)(row + k) * columns;
            int32_t sum = 0;
            uint16_t lanes[8] = {0};
            for (int g = 0; g < groups; g++) {
                for (int lane = 0; lane < 8; lane++) {
                    int i = g * 8 + lane;
                    lanes[lane] += (int16_t)input[i] * (int16_t)w[i];
                }
            }
            for (int i = 0; i < 8; i++) {
                sum += (int16_t)lanes[i];
            }
            int32_t value = xtts_matrix_convert(sum, bias[row + k],
                                                scale, output_scale);
            output[row + k] = value > INT16_MAX ? INT16_MAX :
                              value < INT16_MIN ? INT16_MIN : value;
        }
#endif
    }
}

static void xtts_matrix_i16_w8(int columns, int rows, const int16_t *input,
                               const int8_t *weights, const int16_t *bias,
                               int16_t *output, int input_q, int weight_q,
                               int bias_q, int output_q)
{
    int count = (columns / 8) * 8;
    float scale = xtts_matrix_scale(bias_q - input_q - weight_q);
    float output_scale = xtts_matrix_scale(output_q - bias_q);

    for (int row = 0; row < rows; row += 2) {
        uint32_t sums[2] = {0, 0};
#if defined(__loongarch64) && !defined(XTTS_MATRIX_SCALAR)
        if (count > 0) {
            const int16_t *x = input;
            const int8_t *w0 = weights + (int64_t)row * columns;
            const int8_t *w1 = w0 + columns;
            int n = count;
            __asm__ volatile (
                "vxor.v $vr3, $vr3, $vr3\n\t"
                "vxor.v $vr4, $vr4, $vr4\n\t"
                "1:\n\t"
                "vld $vr0, %[x], 0\n\t"
                "vldrepl.d $vr1, %[w0], 0\n\t"
                "vldrepl.d $vr2, %[w1], 0\n\t"
                "vsllwil.h.b $vr1, $vr1, 0\n\t"
                "vsllwil.h.b $vr2, $vr2, 0\n\t"
                "vmaddwev.w.h $vr3, $vr0, $vr1\n\t"
                "vmaddwod.w.h $vr3, $vr0, $vr1\n\t"
                "vmaddwev.w.h $vr4, $vr0, $vr2\n\t"
                "vmaddwod.w.h $vr4, $vr0, $vr2\n\t"
                "addi.d %[x], %[x], 16\n\t"
                "addi.d %[w0], %[w0], 8\n\t"
                "addi.d %[w1], %[w1], 8\n\t"
                "addi.w %[n], %[n], -8\n\t"
                "bnez %[n], 1b\n\t"
                "vshuf4i.w $vr0, $vr3, 0x4e\n\t"
                "vshuf4i.w $vr1, $vr4, 0x4e\n\t"
                "vadd.w $vr3, $vr3, $vr0\n\t"
                "vadd.w $vr4, $vr4, $vr1\n\t"
                "vshuf4i.w $vr0, $vr3, 0xb1\n\t"
                "vshuf4i.w $vr1, $vr4, 0xb1\n\t"
                "vadd.w $vr3, $vr3, $vr0\n\t"
                "vadd.w $vr4, $vr4, $vr1\n\t"
                "vpickve2gr.w %[a], $vr3, 0\n\t"
                "vpickve2gr.w %[b], $vr4, 0\n\t"
                : [x] "+&r"(x), [w0] "+&r"(w0), [w1] "+&r"(w1),
                  [n] "+&r"(n), [a] "=&r"(sums[0]), [b] "=&r"(sums[1])
                : : "f0", "f1", "f2", "f3", "f4", "memory");
        }
#else
        for (int i = 0; i < count; i++) {
            sums[0] += (uint32_t)((int32_t)input[i] *
                                  weights[(int64_t)row * columns + i]);
            sums[1] += (uint32_t)((int32_t)input[i] *
                                  weights[(int64_t)(row + 1) * columns + i]);
        }
#endif
        for (int k = 0; k < 2; k++) {
            int32_t value = xtts_matrix_convert(sums[k], bias[row + k],
                                                scale, output_scale);
            output[row + k] = value > INT16_MAX ? INT16_MAX :
                              value < INT16_MIN ? INT16_MIN : value;
        }
    }
}

#endif
