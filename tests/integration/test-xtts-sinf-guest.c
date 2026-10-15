/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* Run natively on x86 and under LATX, then compare the complete output files. */
extern void capture_sinf(uint32_t input, uint64_t *output, uintptr_t entry);
__asm__(
    ".text\n"
    ".global capture_sinf\n"
    ".type capture_sinf,@function\n"
    "capture_sinf:\n"
    "push %rbx\n"
    "mov %rsi,%rbx\n"
    "movd %edi,%xmm0\n"
    "mov %rdx,%rax\n"
    "call *%rax\n"
    "mov %rax,0(%rbx)\n"
    "mov %rdx,8(%rbx)\n"
    "mov %rcx,16(%rbx)\n"
    "mov %rsi,24(%rbx)\n"
    "mov %rdi,32(%rbx)\n"
    "pushfq\n"
    "pop %rax\n"
    "and $0x8c5,%eax\n"
    "mov %rax,40(%rbx)\n"
    "movdqu %xmm0,48(%rbx)\n"
    "movdqu %xmm1,64(%rbx)\n"
    "movdqu %xmm2,80(%rbx)\n"
    "movdqu %xmm3,96(%rbx)\n"
    "pop %rbx\n"
    "ret\n"
    ".size capture_sinf,.-capture_sinf\n");

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) return 2;
    uintptr_t entry = argc == 3 ? UINT64_C(0x551818ae20) :
                                  UINT64_C(0x551818a770);
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)UINT64_C(0x5518000000);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) {
        perror("mmap");
        return 4;
    }
    close(fd);
    uint32_t state = 12345;
    for (int i = 0; i < 20000; i++) {
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        uint32_t bits = (state & 0x807fffff) | ((state % 146 + 1) << 23);
        uint64_t result[14];
        capture_sinf(bits, result, entry);
        if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
    }
    static const uint32_t special[] = {
        0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x807fffff,
        0x00800000, 0x80800000, 0x497fffff, 0xc97fffff,
        0x49800000, 0xc9800000, 0x7f7fffff, 0xff7fffff,
        0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000,
    };
    for (unsigned i = 0; i < sizeof(special) / sizeof(special[0]); i++) {
        uint64_t result[14];
        capture_sinf(special[i], result, entry);
        if (fwrite(result, sizeof(result), 1, stdout) != 1) return 5;
    }
    return 0;
}
