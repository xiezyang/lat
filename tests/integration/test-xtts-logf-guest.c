/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern void capture_logf(uint32_t input, uint64_t *output, uintptr_t entry);
__asm__(
    ".text\n.global capture_logf\n.type capture_logf,@function\n"
    "capture_logf:\n push %rbx\n mov %rsi,%rbx\n"
    "pcmpeqd %xmm0,%xmm0\n pinsrd $0,%edi,%xmm0\n"
    "call *%rdx\n"
    "mov %rax,0(%rbx)\n mov %rdx,8(%rbx)\n mov %rcx,16(%rbx)\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n mov %rax,24(%rbx)\n"
    "movdqu %xmm0,32(%rbx)\n movdqu %xmm1,48(%rbx)\n"
    "movdqu %xmm2,64(%rbx)\n movdqu %xmm3,80(%rbx)\n"
    "movdqu %xmm4,96(%rbx)\n"
    "pop %rbx\n ret\n.size capture_logf,.-capture_logf\n");

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
    uint32_t state = 23456;
    for (int fn = 0; fn < 2; fn++) {
        uintptr_t entry = fn ? UINT64_C(0x551818b050) :
                               UINT64_C(0x551818ae40);
        for (int i = 0; i < 20000; i++) {
            state = state * UINT32_C(1664525) + UINT32_C(1013904223);
            uint32_t input = (state & 0x007fffff) |
                             ((state % 254 + 1) << 23);
            uint64_t result[14];
            capture_logf(input, result, entry);
            if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
        }
        static const uint32_t special[] = {
            0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x807fffff,
            0x00800000, 0x80800000, 0x3f800000, 0x7f7fffff,
            0xff7fffff, 0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000,
        };
        for (unsigned i = 0; i < sizeof(special) / sizeof(special[0]); i++) {
            uint64_t result[14];
            capture_logf(special[i], result, entry);
            if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
        }
    }
    return 0;
}
