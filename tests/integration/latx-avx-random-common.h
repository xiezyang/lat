/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef LATX_AVX_RANDOM_COMMON_H
#define LATX_AVX_RANDOM_COMMON_H

#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

enum {
    LATX_AVX_RANDOM_DEFAULT_ROUNDS = 1024,
    LATX_AVX_RANDOM_MAX_ROUNDS = 100000,
    LATX_AVX_RANDOM_OUTPUT_SIZE = 1024,
};

typedef void (*latx_avx_random_case_fn)(const uint8_t *input_a,
                                        const uint8_t *input_b,
                                        uint8_t *output,
                                        uint8_t *page_edge,
                                        uintptr_t index);

struct latx_avx_random_buffers {
    _Alignas(64) uint8_t input_a[1024];
    _Alignas(64) uint8_t input_b[1024];
    _Alignas(64) uint8_t output[LATX_AVX_RANDOM_OUTPUT_SIZE];
};

static uint64_t latx_avx_random_next(uint64_t *state)
{
    uint64_t value = *state;

    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    *state = value;
    return value * UINT64_C(2685821657736338717);
}

static void latx_avx_random_fill(uint8_t *buffer, size_t size,
                                 unsigned int round, uint64_t *state)
{
    for (size_t i = 0; i < size; ++i) {
        switch (round & 7U) {
        case 0:
            buffer[i] = 0x00;
            break;
        case 1:
            buffer[i] = 0xff;
            break;
        case 2:
            buffer[i] = (uint8_t)(UINT8_C(1) << (i & 7U));
            break;
        case 3:
            buffer[i] = (uint8_t)i;
            break;
        case 4:
            buffer[i] = (i & 1U) ? 0xaa : 0x55;
            break;
        default:
            buffer[i] = (uint8_t)latx_avx_random_next(state);
            break;
        }
    }
}

static int latx_avx_random_write_all(const void *buffer, size_t size)
{
    const uint8_t *bytes = buffer;

    while (size != 0) {
        ssize_t written = write(STDOUT_FILENO, bytes, size);

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        bytes += written;
        size -= (size_t)written;
    }
    return 0;
}

static unsigned int latx_avx_random_rounds(int argc, char **argv)
{
    char *end;
    unsigned long parsed;

    if (argc == 1) {
        return LATX_AVX_RANDOM_DEFAULT_ROUNDS;
    }
    if (argc != 2) {
        fprintf(stderr, "usage: %s [rounds]\n", argv[0]);
        exit(2);
    }

    errno = 0;
    parsed = strtoul(argv[1], &end, 10);
    if (errno != 0 || *argv[1] == '\0' || *end != '\0' || parsed == 0 ||
        parsed > LATX_AVX_RANDOM_MAX_ROUNDS) {
        fprintf(stderr, "rounds must be between 1 and %u\n",
                LATX_AVX_RANDOM_MAX_ROUNDS);
        exit(2);
    }
    return (unsigned int)parsed;
}

static int latx_avx_random_main(const char *name, latx_avx_random_case_fn fn,
                                int argc, char **argv)
{
    struct latx_avx_random_buffers buffers;
    const unsigned int rounds = latx_avx_random_rounds(argc, argv);
    const long page_size = sysconf(_SC_PAGESIZE);
    uint8_t *mapping;
    uint8_t *page_edge;
    uint64_t state = UINT64_C(0x6a09e667f3bcc909);
    char header[96];
    int header_size;

    if (page_size < 4096) {
        fprintf(stderr, "invalid page size: %ld\n", page_size);
        return 2;
    }

    mapping = mmap(NULL, page_size * 2, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED) {
        perror("mmap");
        return 2;
    }
    page_edge = mapping + page_size - 32;

    header_size = snprintf(header, sizeof(header),
                           "LATX_AVX_RANDOM_V1 name=%s rounds=%u output=%u\n",
                           name, rounds, LATX_AVX_RANDOM_OUTPUT_SIZE);
    if (header_size < 0 || (size_t)header_size >= sizeof(header) ||
        latx_avx_random_write_all(header, (size_t)header_size) != 0) {
        perror("write");
        munmap(mapping, page_size * 2);
        return 2;
    }

    for (unsigned int round = 0; round < rounds; ++round) {
        latx_avx_random_fill(buffers.input_a, sizeof(buffers.input_a), round,
                             &state);
        latx_avx_random_fill(buffers.input_b, sizeof(buffers.input_b),
                             round + 3U, &state);
        latx_avx_random_fill(buffers.output, sizeof(buffers.output),
                             round + 5U, &state);
        latx_avx_random_fill(page_edge - 128, 256, round + 7U, &state);

        fn(buffers.input_a, buffers.input_b, buffers.output, page_edge, 16);

        if (latx_avx_random_write_all(buffers.output,
                                      sizeof(buffers.output)) != 0) {
            perror("write");
            munmap(mapping, page_size * 2);
            return 2;
        }
    }

    munmap(mapping, page_size * 2);
    return 0;
}

#endif
