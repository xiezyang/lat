/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef LATX_AVX_SINGLE_COMMON_H
#define LATX_AVX_SINGLE_COMMON_H

#include <stddef.h>
#include <stdint.h>

enum {
    LATX_AVX_SINGLE_SYS_WRITE = 1,
    LATX_AVX_SINGLE_STDOUT = 1,
};

static inline long latx_avx_single_syscall3(long number, long arg0,
                                            long arg1, long arg2)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = arg0;
    register long rsi __asm__("rsi") = arg1;
    register long rdx __asm__("rdx") = arg2;

    __asm__ volatile("syscall"
                     : "+a"(rax)
                     : "D"(rdi), "S"(rsi), "d"(rdx)
                     : "rcx", "r11", "memory");
    return rax;
}

static inline int latx_avx_single_write_all(const void *buffer, size_t size)
{
    const uint8_t *bytes = buffer;

    while (size != 0) {
        long written = latx_avx_single_syscall3(
            LATX_AVX_SINGLE_SYS_WRITE, LATX_AVX_SINGLE_STDOUT,
            (long)(uintptr_t)bytes, (long)size);

        if (written <= 0) {
            return -1;
        }
        bytes += written;
        size -= (size_t)written;
    }
    return 0;
}

static inline void latx_avx_single_fill(uint8_t *buffer, size_t size,
                                        uint64_t seed)
{
    uint64_t state = seed;

    for (size_t i = 0; i < size; ++i) {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        buffer[i] = (uint8_t)(state * UINT64_C(2685821657736338717));
    }
}

int latx_avx_single_main(long argc, char **argv);

#endif
