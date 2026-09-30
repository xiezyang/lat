#define _GNU_SOURCE

#include <dlfcn.h>
#include <inttypes.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef int32_t (*moddiff_fn)(int16_t value, int16_t gain);

static uint64_t elapsed_ns(const struct timespec *start,
                           const struct timespec *end)
{
    return (uint64_t)(end->tv_sec - start->tv_sec) * UINT64_C(1000000000) +
           (uint64_t)(end->tv_nsec - start->tv_nsec);
}

int main(int argc, char **argv)
{
    void *cpp, *handle;
    struct link_map *map;
    moddiff_fn fn;
    uint64_t iterations;
    uint64_t sum = 0;
    struct timespec start, end;

    if (argc != 3) {
        fprintf(stderr, "usage: %s LIB ITERATIONS\n", argv[0]);
        return 2;
    }
    iterations = strtoull(argv[2], NULL, 0);
    cpp = dlopen("libstdc++.so.6", RTLD_LAZY | RTLD_GLOBAL);
    handle = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!cpp || !handle || dlinfo(handle, RTLD_DI_LINKMAP, &map)) {
        fprintf(stderr, "dlopen/dlinfo: %s\n", dlerror());
        return 1;
    }
    fn = (moddiff_fn)(map->l_addr + 0x1e0d70);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
    for (uint64_t i = 0; i < iterations; i++) {
        int32_t value = (int16_t)(i * 257 + 0x3000);
        int32_t gain = (int16_t)(i * 37 + 0x1234);
        sum += (uint32_t)fn((int16_t)gain, (int16_t)value);
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    printf("iterations=%" PRIu64 " elapsed_ns=%" PRIu64
           " checksum=%" PRIu64 "\n", iterations, elapsed_ns(&start, &end),
           sum);
    return 0;
}
