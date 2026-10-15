/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern void capture_expf(uint32_t input, uint64_t *output);
__asm__(
    ".text\n.global capture_expf\n.type capture_expf,@function\n"
    "capture_expf:\n"
    "push %rbx\n mov %rsi,%rbx\n"
    "pcmpeqd %xmm0,%xmm0\n pinsrd $0,%edi,%xmm0\n"
    "movabs $0x551818abe0,%rax\n call *%rax\n"
    "mov %rax,0(%rbx)\n mov %rdx,8(%rbx)\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n mov %rax,16(%rbx)\n"
    "movdqu %xmm0,24(%rbx)\n movdqu %xmm1,40(%rbx)\n"
    "movdqu %xmm2,56(%rbx)\n movdqu %xmm3,72(%rbx)\n"
    "movdqu %xmm4,88(%rbx)\n"
    "pop %rbx\n ret\n.size capture_expf,.-capture_expf\n");

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)UINT64_C(0x5518000000);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    uint32_t state = 12345;
    for (int i = 0; i < 20000; i++) {
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        float input = ((int32_t)(state % 1600001) - 800000) / 10000.0f;
        union { float f; uint32_t u; } bits = { .f = input };
        uint64_t result[13];
        capture_expf(bits.u, result);
        if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
    }
    static const uint32_t special[] = {
        0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x807fffff,
        0x00800000, 0x80800000, 0x429fffff, 0xc29fffff,
        0x42a00000, 0xc2a00000, 0x7f7fffff, 0xff7fffff,
        0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000,
    };
    for (unsigned i = 0; i < sizeof(special) / sizeof(special[0]); i++) {
        uint64_t result[13];
        capture_expf(special[i], result);
        if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
    }
    return 0;
}
