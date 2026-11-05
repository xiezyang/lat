/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct mpi { int sign; size_t count; uint64_t *limbs; };
extern void capture_compare(const struct mpi *, const struct mpi *, uint64_t *,
                            uintptr_t);
__asm__(
    ".text\n.global capture_compare\n.type capture_compare,@function\n"
    "capture_compare:\n push %r12\n mov %rdx,%r12\n"
    "mov $0x12345678,%edx\n mov $0x87654321,%ecx\n"
    "mov %r9,%rax\n call *%rax\n"
    "mov %rax,0(%r12)\n mov %rdx,8(%r12)\n mov %rcx,16(%r12)\n"
    "pushfq\n pop %rax\n and $0x8c5,%eax\n mov %rax,24(%r12)\n"
    "pop %r12\n ret\n.size capture_compare,.-capture_compare\n");

static uint64_t next(uint64_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 7; *s ^= *s << 17;
    return *s;
}

int main(int argc, char **argv)
{
    struct stat st;
    if (argc != 2 && argc != 3) return 2;
    long repeat = 1;
    const char *setting = getenv("LATX_BENCH_REPEAT");
    if (setting) {
        char *end;
        errno = 0;
        repeat = strtol(setting, &end, 10);
        if (errno || end == setting || *end || repeat < 1 || repeat > 10000)
            return 2;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0 || fstat(fd, &st)) return 3;
    int engine = argc == 3 && !strcmp(argv[2], "engine");
    void *base = (void *)(engine ? UINT64_C(0x5518000000) :
                                  UINT64_C(0x5508200000));
    uintptr_t entry = engine ? UINT64_C(0x55182e7faa) :
                               UINT64_C(0x550846a454);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    struct mpi *x = mmap((void *)UINT64_C(0x6000000000), 8192,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                         -1, 0);
    if (x == MAP_FAILED) return 5;
    struct mpi *y = x + 1;
    x->limbs = (uint64_t *)(x + 2); y->limbs = x->limbs + 256;
    uint64_t state = 874129;
    for (int t = 0; t < 20000; t++) {
        x->sign = t & 1 ? -1 : 1; y->sign = t & 2 ? -1 : 1;
        x->count = t % 257; y->count = t % 3 ? x->count : next(&state) % 257;
        for (int i = 0; i < 256; i++) {
            x->limbs[i] = i < t % 257 ? next(&state) : 0;
            y->limbs[i] = t % 5 ? x->limbs[i] : next(&state);
            if (t % 7 == 0) x->limbs[i] = y->limbs[i] = 0;
        }
        uint64_t out[4];
        for (long r = 0; r < repeat; r++) capture_compare(x, y, out, entry);
        if (fwrite(out, sizeof(out), 1, stdout) != 1) return 6;
    }
    return 0;
}
