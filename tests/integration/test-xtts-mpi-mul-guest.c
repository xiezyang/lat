/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern void capture_mul(uint64_t, const uint64_t *, uint64_t *, uint64_t,
                        uintptr_t);
__asm__(
    ".text\n.global capture_mul\n.type capture_mul,@function\n"
    "capture_mul:\n push %r12\n"
    "mov %r8,%rax\n call *%rax\n"
    "pop %r12\n ret\n.size capture_mul,.-capture_mul\n");

static uint64_t next(uint64_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

int main(int argc, char **argv)
{
    struct stat st;
    if (argc != 2 && argc != 3) return 2;
    int engine = argc == 3 && !strcmp(argv[2], "engine");
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)(engine ? UINT64_C(0x5518000000) :
                                  UINT64_C(0x5508200000));
    uintptr_t entry = engine ? UINT64_C(0x55182e7faa) :
                               UINT64_C(0x550846ae71);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);

    uint64_t *arena = mmap((void *)UINT64_C(0x6000000000), 65536,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                           -1, 0);
    if (arena == MAP_FAILED) return 5;
    uint64_t seed = 0x9e3779b97f4a7c15ULL;
    for (int t = 0; t < 20000; t++) {
        size_t n = t < 65 ? (size_t)t : 1 + next(&seed) % 64;
        uint64_t b = (t % 11 == 0) ? UINT64_MAX - (t & 3) : next(&seed);
        uint64_t *s = arena;
        uint64_t *d = arena + 80;
        for (size_t i = 0; i < n; i++) s[i] = next(&seed);
        for (size_t i = 0; i < n + 2; i++) d[i] = next(&seed);
        capture_mul(n, s, d, b, entry);
        if (fwrite(d, sizeof(*d), n + 2, stdout) != (n + 2)) return 6;
    }
    return 0;
}
