/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern void capture_powf(uint32_t base, uint32_t power, uint64_t *output);
__asm__(
    ".text\n.global capture_powf\n.type capture_powf,@function\n"
    "capture_powf:\n push %rbx\n mov %rdx,%rbx\n"
    "movd %edi,%xmm0\n pcmpeqd %xmm1,%xmm1\n pinsrd $0,%esi,%xmm1\n"
    "movabs $0x551818aca0,%rax\n call *%rax\n"
    "mov %rax,0(%rbx)\n mov %rdx,8(%rbx)\n mov %rcx,16(%rbx)\n"
    "mov %rsi,24(%rbx)\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n mov %rax,32(%rbx)\n"
    "movdqu %xmm0,40(%rbx)\n movdqu %xmm1,56(%rbx)\n"
    "movdqu %xmm2,72(%rbx)\n movdqu %xmm3,88(%rbx)\n"
    "movdqu %xmm4,104(%rbx)\n movdqu %xmm5,120(%rbx)\n"
    "pop %rbx\n ret\n.size capture_powf,.-capture_powf\n");

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) return 2;
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)UINT64_C(0x5518000000);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    uint32_t state = 54321;
    for (int i = 0; i < 20000; i++) {
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        uint32_t input = (state & 0x007fffff) | ((state % 128 + 63) << 23);
        union { float f; uint32_t u; } power = {
            .f = ((int32_t)(state % 1600001) - 800000) / 10000.0f
        };
        uint64_t result[17];
        capture_powf(input, power.u, result);
        if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
    }
    if (argc == 3) {
        static const uint32_t special[] = {
            0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x807fffff,
            0x00800000, 0x1f7fffff, 0x1f800000, 0x3f800000,
            0x5f7fffff, 0x5f800000, 0x7f7fffff, 0xff7fffff,
            0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000,
        };
        for (unsigned i = 0; i < sizeof(special) / sizeof(special[0]); i++) {
            uint64_t result[17];
            capture_powf(special[i], 0x3f000000, result);
            if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
            capture_powf(0x3fc00000, special[i], result);
            if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
        }
    }
    return 0;
}
