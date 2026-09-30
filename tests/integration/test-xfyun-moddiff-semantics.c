#define _GNU_SOURCE

#include <dlfcn.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int32_t (*moddiff_fn)(int16_t value, int16_t gain);

static int32_t reference_moddiff(int16_t value, int16_t gain)
{
    int32_t x = value;
    int32_t y = gain;
    int32_t result;

    if (x <= 0 || x > 0x7a3b) {
        result = INT16_MIN;
    } else if (x <= 0x65db) {
        result = 0x599a;
    } else {
        int32_t shifted = (x - 0x65dc) << 15;
        int64_t product = (int64_t)shifted * INT32_C(0x06488dc5);
        int32_t quotient = (int32_t)(product >> 32);

        quotient >>= 7;
        quotient -= shifted >> 31;
        result = (int16_t)quotient;
        result = (int16_t)(((result * 0x2666) >> 15) + 0x599a);
    }
    return (result * y) >> 15;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] :
        "/tmp/xfyun-board200-libmsc-20260929.so";
    void *cpp = dlopen("libstdc++.so.6", RTLD_LAZY | RTLD_GLOBAL);
    void *handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    struct link_map *map;
    moddiff_fn original;
    uint32_t state = 0x91e10da5;
    const int32_t boundaries[] = {
        INT16_MIN, -1, 0, 1, 0x65da, 0x65db, 0x65dc, 0x65dd,
        0x7a39, 0x7a3a, 0x7a3b, 0x7a3c, INT16_MAX,
    };

    if (!cpp || !handle || dlinfo(handle, RTLD_DI_LINKMAP, &map)) {
        fprintf(stderr, "dlopen/dlinfo: %s\n", dlerror());
        return 1;
    }
    original = (moddiff_fn)(map->l_addr + 0x1e0d70);

    for (size_t i = 0; i < sizeof(boundaries) / sizeof(boundaries[0]); i++) {
        for (int32_t gain = INT16_MIN; gain <= INT16_MAX; gain++) {
            int16_t v = (int16_t)boundaries[i];
            int32_t actual = original((int16_t)gain, v);
            int32_t expected = reference_moddiff(v, (int16_t)gain);

            if (actual != expected) {
                fprintf(stderr, "boundary mismatch value=%d gain=%d "
                        "actual=%d expected=%d\n", v, gain,
                        actual, expected);
                return 1;
            }
        }
    }

    for (int test = 0; test < 1000000; test++) {
        int16_t value = (int16_t)rand_r(&state);
        int16_t gain = (int16_t)rand_r(&state);
        int32_t actual = original(gain, value);
        int32_t expected = reference_moddiff(value, gain);

        if (actual != expected) {
            fprintf(stderr, "random mismatch value=%d gain=%d "
                    "actual=%d expected=%d\n", value, gain,
                    actual, expected);
            return 1;
        }
    }
    puts("ok");
    return 0;
}
