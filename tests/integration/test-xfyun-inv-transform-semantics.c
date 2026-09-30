#define _GNU_SOURCE

#include <dlfcn.h>
#include <inttypes.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int32_t (*iv_fn)(uint32_t input);
typedef void (*transform_fn)(const int16_t *input, int32_t *output);

static int32_t interp_table(const int16_t *table, uint16_t value)
{
    uint32_t index = value & 0x7f;
    uint32_t offset = value >> 7;

    if (value & 0x8000) {
        index = 0x80 - index;
        offset = 0x1ff - offset;
    }
    return ((int32_t)table[offset] * (int32_t)(0x80 - index) +
            (int32_t)table[offset + 1] * (int32_t)index) >> 7;
}

static int64_t scale_high(int64_t value, int16_t input)
{
    int64_t high = value >> 14;
    uint32_t low = (uint32_t)value & 0x3fff;
    int32_t low_product = (int32_t)((uint32_t)((int32_t)input *
                                   (int32_t)low));

    return high * input + (low_product >> 14);
}

static int64_t scale_low(int64_t value, int16_t input)
{
    return (value * input + 0x2000) >> 14;
}

static void transform_reference(const int16_t *table, const int16_t *input,
                                int32_t *output)
{
    int16_t neg[40];
    int64_t a[41] = {0};
    int64_t b[41] = {0};

    for (int i = 0; i < 40; i++) {
        neg[i] = (int16_t)(-(uint32_t)interp_table(table, input[i]));
    }
    a[0] = UINT64_C(0x400000000);
    b[0] = UINT64_C(0x400000000);
    a[1] = (int64_t)neg[0] << 20;
    b[1] = (int64_t)neg[1] << 20;
    a[2] = UINT64_C(0x400000000);
    b[2] = UINT64_C(0x400000000);

    for (int k = 1; k <= 19; k++) {
        int16_t ka = neg[2 * k];
        int16_t kb = neg[2 * k + 1];

        for (int i = 2 * k + 2; i >= 2; i--) {
            int64_t da = k <= 9 ? scale_low(a[i - 1], ka) :
                                  scale_high(a[i - 1], ka);
            int64_t db = k <= 9 ? scale_low(b[i - 1], kb) :
                                  scale_high(b[i - 1], kb);

            a[i] += a[i - 2] + da;
            b[i] += b[i - 2] + db;
        }
        a[1] += k <= 9 ? scale_low(a[0], ka) : scale_high(a[0], ka);
        b[1] += k <= 9 ? scale_low(b[0], kb) : scale_high(b[0], kb);
    }

    for (int i = 20; i >= 1; i--) {
        a[i] += a[i - 1];
        b[i] -= b[i - 1];
    }
    for (int i = 0; i < 20; i++) {
        output[i] = (int32_t)((b[i + 1] - a[i + 1] + 0x80000) >> 20);
        output[20 + i] =
            (int32_t)((-b[20 - i] - a[20 - i] + 0x80000) >> 20);
    }
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] :
        "/tmp/xfyun-board200-libmsc-20260929.so";
    void *handle, *cpp;
    struct link_map *map;
    uintptr_t base;
    iv_fn iv;
    transform_fn transform;
    const int16_t *table;
    uint32_t state = 0x6d2b79f5;

    cpp = dlopen("libstdc++.so.6", RTLD_LAZY | RTLD_GLOBAL);
    handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!cpp || !handle) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map)) {
        fprintf(stderr, "dlinfo: %s\n", dlerror());
        return 1;
    }
    base = map->l_addr;
    iv = (iv_fn)(base + 0x1c84e0);
    transform = (transform_fn)(base + 0x1bee10);
    table = *(const int16_t **)(base + 0x607fd8);

    for (uint32_t value = 0; value <= UINT16_MAX; value++) {
        if (iv(value) != interp_table(table, (uint16_t)value)) {
            fprintf(stderr, "interp mismatch value=%u\n", value);
            return 1;
        }
    }

    for (int test = 0; test < 200000; test++) {
        int16_t input[40];
        int32_t original[40];
        int32_t expected[40];

        for (int i = 0; i < 40; i++) {
            input[i] = (int16_t)rand_r(&state);
            original[i] = (int32_t)rand_r(&state);
        }
        memcpy(expected, original, sizeof(expected));
        transform(input, original);
        transform_reference(table, input, expected);
        if (memcmp(original, expected, sizeof(original))) {
            fprintf(stderr, "transform mismatch test=%d\n", test);
            for (int i = 0; i < 40; i++) {
                if (original[i] != expected[i]) {
                    fprintf(stderr, "i=%d original=%d expected=%d\n",
                            i, original[i], expected[i]);
                }
            }
            return 1;
        }
    }
    puts("ok");
    return 0;
}
