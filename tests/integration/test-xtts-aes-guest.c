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

struct aes { int rounds; uint32_t *keys; };
extern void capture_aes(struct aes *, int, const void *, void *, uint64_t *,
                        uintptr_t);
__asm__(
    ".text\n.global capture_aes\n.type capture_aes,@function\n"
    "capture_aes:\n push %r12\n mov %r8,%r12\n"
    "pcmpeqb %xmm2,%xmm2\n movdqa %xmm2,%xmm3\n"
    "movdqa %xmm2,%xmm8\n movdqa %xmm2,%xmm15\n"
    "mov %r9,%rax\n call *%rax\n"
    "mov %rax,0(%r12)\n mov %rdx,8(%r12)\n mov %rcx,16(%r12)\n"
    "mov %rsi,24(%r12)\n mov %rdi,32(%r12)\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n mov %rax,40(%r12)\n"
    "movdqu %xmm0,48(%r12)\n movdqu %xmm1,64(%r12)\n"
    "movdqu %xmm2,80(%r12)\n movdqu %xmm3,96(%r12)\n"
    "movdqu %xmm8,112(%r12)\n movdqu %xmm15,128(%r12)\n"
    "pop %r12\n ret\n.size capture_aes,.-capture_aes\n");

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
    int aikit = argc == 3 && !strcmp(argv[2], "aikit");
    void *base = (void *)(aikit ? UINT64_C(0x5508200000) :
                                  UINT64_C(0x5518000000));
    uintptr_t entry = aikit ? UINT64_C(0x55084b1fb1) :
                              UINT64_C(0x55182d0600);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    uint8_t *arena = mmap((void *)UINT64_C(0x6000000000), 8192,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                          -1, 0);
    if (arena == MAP_FAILED) return 5;
    struct aes *ctx = (struct aes *)arena;
    uint64_t seed = 827419;
    for (int t = 0; t < 10000; t++) {
        for (int i = 64; i < 1024; i++) arena[i] = next(&seed);
        ctx->rounds = 2 + t % 13;
        ctx->keys = (uint32_t *)(arena + 64 + t % 16);
        uint8_t *src = arena + 512 + t % 16;
        uint8_t *dst = t % 3 == 0 ? src : arena + 768 + t % 16;
        uint64_t out[18];
        uint8_t input[16];
        memcpy(input, src, sizeof(input));
        for (long r = 0; r < repeat; r++) {
            /* Keep in-place repeats identical to the single-call case. */
            if (r && src == dst) memcpy(src, input, sizeof(input));
            capture_aes(ctx, t & 1, src, dst, out, entry);
        }
        if (fwrite(out, sizeof(out), 1, stdout) != 1 ||
            fwrite(dst, 16, 1, stdout) != 1) return 6;
    }
    return 0;
}
