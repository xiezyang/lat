#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define LOOPS 2000000

static uint64_t now_ns(void)
{
    struct timespec ts;

    syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main(void)
{
    struct timespec ts;
    uint64_t start = now_ns();
    uint64_t end;
    long checksum = 0;

    for (int i = 0; i < LOOPS; i++) {
        syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts);
        checksum += ts.tv_nsec & 1;
    }
    end = now_ns();
    printf("%llu %ld\n", (unsigned long long)(end - start), checksum);
    return 0;
}
