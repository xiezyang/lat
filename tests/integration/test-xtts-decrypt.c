/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../../target/i386/latx/include/xtts-decrypt-native.h"

#undef LATX_XTTS_DECRYPT_NATIVE_H
#define XTTS_DECRYPT_SCALAR
#define xtts_decrypt_bytes scalar_decrypt
#include "../../target/i386/latx/include/xtts-decrypt-native.h"
#undef xtts_decrypt_bytes

typedef uint64_t (*decrypt_fn)(const uint8_t *, uint32_t, const uint8_t *,
                               uint64_t, uint8_t *);

int main(int argc, char **argv)
{
    decrypt_fn reference = scalar_decrypt;
    if (argc == 2) {
        void *lib = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
        reference = lib ? (decrypt_fn)dlsym(lib, "iDecryptDoProcessV20190001")
                        : NULL;
        if (!reference) {
            fprintf(stderr, "SDK load failed: %s\n", dlerror());
            return 2;
        }
    }
    uint8_t a[16384], b[16384], key[32];
    unsigned state = 76543;
    for (int test = 0; test < 12000; test++) {
        for (unsigned i = 0; i < sizeof(a); i++) a[i] = rand_r(&state);
        memcpy(b, a, sizeof(a));
        for (unsigned i = 0; i < sizeof(key); i++) key[i] = rand_r(&state);
        unsigned key_len = test % 40;
        unsigned len = test < 256 ? test : rand_r(&state) % 4097;
        int delta = test % 257 - 128;
        if (test % 7 == 0) delta = 5000;
        unsigned src = 256 + test % 32;
        if (xtts_decrypt_bytes(key, key_len, a + src, len, a + src + delta) !=
            reference(key, key_len, b + src, len, b + src + delta) ||
            memcmp(a, b, sizeof(a))) {
            fprintf(stderr, "Mismatch: test=%d key=%u len=%u delta=%d\n",
                    test, key_len, len, delta);
            return 1;
        }
    }
    if (xtts_decrypt_bytes(NULL, 0, NULL, 0, NULL) != 1 ||
        xtts_decrypt_bytes(key, 0, NULL, 0, NULL) != 2 ||
        xtts_decrypt_bytes(key, 8, NULL, 0, NULL) != 3 ||
        xtts_decrypt_bytes(key, 8, a, 0, NULL) != 4 ||
        xtts_decrypt_bytes(key, 8, a, 1, NULL) != 5) return 3;

    size_t page = sysconf(_SC_PAGESIZE);
    uint8_t *guard = mmap(NULL, page * 4, PROT_NONE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (guard == MAP_FAILED || mprotect(guard, page, PROT_READ | PROT_WRITE) ||
        mprotect(guard + page * 2, page, PROT_READ | PROT_WRITE)) return 4;
    for (unsigned len = 1; len <= 256; len++) {
        uint8_t *input = guard + page - len;
        uint8_t *output = guard + page * 3 - len;
        memset(input, 0x39, len);
        if (xtts_decrypt_bytes(key, 32, input, len, output) ||
            reference(key, 32, input, len, b) || memcmp(output, b, len)) {
            return 5;
        }
    }
    munmap(guard, page * 4);
    puts("PASS: 12000 decrypt comparisons, overlap, errors and guard pages");
    return 0;
}
