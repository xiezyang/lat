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

extern void capture_accumulate(float *, float *, int, uint64_t *);
__asm__(
    ".text\n.global capture_accumulate\n.type capture_accumulate,@function\n"
    "capture_accumulate:\n push %rbx\n push %r15\n push %r12\n"
    "mov %rcx,%r12\n mov %rdi,%r15\n mov %rsi,%r9\n mov %edx,%ebx\n"
    "mov $0x3eaaaaab,%eax\n movd %eax,%xmm1\n"
    "xor %eax,%eax\n xor %edx,%edx\n"
    "movabs $0x55182bedb8,%rcx\n call *%rcx\n"
    "mov %rax,0(%r12)\n mov %rdx,8(%r12)\n mov %rcx,16(%r12)\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n mov %rax,24(%r12)\n"
    "movdqu %xmm0,32(%r12)\n movdqu %xmm1,48(%r12)\n"
    "pop %r12\n pop %r15\n pop %rbx\n ret\n"
    ".size capture_accumulate,.-capture_accumulate\n");

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) return 2;
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)UINT64_C(0x5518000000);
    if (mmap(base, st.st_size, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    *(uint8_t *)UINT64_C(0x55182bedfd) = 0xc3;
    unsigned state = 66293;
    for (int test = 0; test < 10000; test++) {
        float storage[2048] __attribute__((aligned(16)));
        int n = rand_r(&state) % 256 + 1;
        for (int i = 0; i < 2048; i++) {
            uint32_t bits = rand_r(&state);
            if (argc == 2 || test % 5) {
                bits = (bits & 0x807fffff) | 0x3f000000;
            }
            memcpy(&storage[i], &bits, sizeof(bits));
        }
        float *destination = storage + 16;
        float *source = test % 3 ? storage + 1024 : destination + test % 23;
        uint64_t captured[8];
        capture_accumulate(destination, source, n, captured);
        if (fwrite(captured, sizeof(captured), 1, stdout) != 1 ||
            fwrite(storage, sizeof(storage), 1, stdout) != 1) return 5;
    }
    return 0;
}
