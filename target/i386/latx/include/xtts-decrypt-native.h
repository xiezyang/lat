/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LATX_XTTS_DECRYPT_NATIVE_H
#define LATX_XTTS_DECRYPT_NATIVE_H

#include <stdint.h>

static uint64_t xtts_decrypt_bytes(const uint8_t *key, uint32_t key_len,
                                  const uint8_t *input, uint64_t len,
                                  uint8_t *output)
{
    static const uint8_t table[] =
        "IFLYTEK CO.,LTD.(iFLYTEK)is a national key software enterprise dedicated to the research of intelligent speech and language technologies, development of software and chip products, provision of speech information services, and integration of E-government s";
    uint8_t stream[32] __attribute__((aligned(16)));
    uint64_t i = 0;

    if (!key) {
        return 1;
    }
    if (key_len - 8 > 0x18) {
        return 2;
    }
    if (!input) {
        return 3;
    }
    if (!len) {
        return 4;
    }
    if (!output) {
        return 5;
    }
    if (key[0] & 1) {
        uint8_t checksum = 0;
        for (uint32_t j = 0; j < key_len; j++) {
            checksum ^= key[j];
        }
        for (uint32_t j = 0; j < 32; j++) {
            stream[j] = table[checksum] ^ key[j % key_len];
            checksum += 2;
        }
    } else {
        uint8_t checksum = 0;
        for (uint32_t j = 0; j < key_len; j++) {
            checksum += key[j];
        }
        for (uint32_t j = 0; j < 32; j++) {
            stream[j] = table[(uint8_t)(checksum + j)] ^ key[j % key_len];
        }
    }
#if defined(__loongarch64) && !defined(XTTS_DECRYPT_SCALAR)
    /* Forward-overlapping output must retain byte-by-byte propagation. */
    if (len >= 32 && ((uintptr_t)output <= (uintptr_t)input ||
                     (uintptr_t)output - (uintptr_t)input >= len)) {
        const uint8_t *src = input;
        uint8_t *dst = output;
        uint64_t blocks = len / 32;
        i = blocks * 32;
        __asm__ volatile (
            "vld $vr0, %[key], 0\n\t"
            "vld $vr1, %[key], 16\n\t"
            "1:\n\t"
            "vld $vr2, %[src], 0\n\t"
            "vld $vr3, %[src], 16\n\t"
            "vxor.v $vr2, $vr2, $vr0\n\t"
            "vxor.v $vr3, $vr3, $vr1\n\t"
            "vst $vr2, %[dst], 0\n\t"
            "vst $vr3, %[dst], 16\n\t"
            "addi.d %[src], %[src], 32\n\t"
            "addi.d %[dst], %[dst], 32\n\t"
            "addi.d %[n], %[n], -1\n\t"
            "bnez %[n], 1b\n\t"
            : [src] "+&r"(src), [dst] "+&r"(dst), [n] "+&r"(blocks)
            : [key] "r"(stream)
            : "f0", "f1", "f2", "f3", "memory");
    }
#endif
    for (; i < len; i++) {
        output[i] = input[i] ^ stream[i & 31];
    }
    return 0;
}

#endif
