#define _GNU_SOURCE

#include <dlfcn.h>
#include <elf.h>
#include <inttypes.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*fir_fn)(int32_t *coeff, int32_t *buf, uint32_t start,
                       uint32_t count);

static void reference(int32_t *coeff, int32_t *buf, uint32_t start,
                      uint32_t count)
{
    int32_t *out = buf + start;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t acc = 0;
        int32_t *in = out + i - 40;

        for (uint32_t j = 0; j < 40; j++) {
            int64_t product = (int64_t)(int16_t)in[j] * coeff[j];
            acc += (uint32_t)product;
        }
        int16_t inc = (int16_t)((int32_t)acc >> 15);
        out[i] = (int32_t)((uint32_t)out[i] + (uint32_t)(int32_t)inc);
    }
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] :
        "/tmp/xfyun-board200-libmsc-20260929.so";
    void *handle;
    struct link_map *map;
    uintptr_t lib_base;
    fir_fn guest;
    uint32_t state = 0x12345678;

    if (!dlopen("libstdc++.so.6", RTLD_LAZY | RTLD_GLOBAL)) {
        fprintf(stderr, "dlopen libstdc++: %s\n", dlerror());
        return 1;
    }
    handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map)) {
        fprintf(stderr, "dlinfo: %s\n", dlerror());
        return 1;
    }
    lib_base = map->l_addr;
    if (!lib_base) {
        fprintf(stderr, "libmsc.so base not found\n");
        return 1;
    }
    guest = (fir_fn)(lib_base + 0x1be6d0);

    for (int test = 0; test < 200000; test++) {
        int32_t coeff[40];
        int32_t original[256];
        int32_t expected[256];
        uint32_t start = 40 + (rand_r(&state) % 120);
        uint32_t count = rand_r(&state) % (200 - start);

        for (int i = 0; i < 40; i++) {
            coeff[i] = (int32_t)rand_r(&state);
        }
        for (int i = 0; i < 256; i++) {
            original[i] = (int32_t)rand_r(&state);
        }
        memcpy(expected, original, sizeof(original));
        guest(coeff, original, start, count);
        reference(coeff, expected, start, count);
        if (memcmp(original, expected, sizeof(original))) {
            fprintf(stderr, "mismatch test=%d start=%u count=%u\n",
                    test, start, count);
            return 1;
        }
    }
    puts("ok");
    return 0;
}
