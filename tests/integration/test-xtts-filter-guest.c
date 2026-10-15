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

extern void capture_filter(float *start, float *end, uint64_t *output);
__asm__(
    ".text\n.global capture_filter\n.type capture_filter,@function\n"
    "capture_filter:\n push %rbx\n mov %rdx,%rbx\n"
    "mov %rdi,%rdx\n mov %rsi,%rcx\n"
    "pcmpeqd %xmm0,%xmm0\n movss -4(%rdx),%xmm7\n"
    "movss %xmm7,%xmm0\n"
    "mov $0x3eaaaaab,%eax\n movd %eax,%xmm3\n"
    "movabs $0x55182bf8b0,%rax\n call *%rax\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n mov %rax,56(%rbx)\n"
    "sub %rdi,%rdx\n mov %rdx,0(%rbx)\n"
    "movdqu %xmm0,8(%rbx)\n movdqu %xmm1,24(%rbx)\n"
    "movdqu %xmm2,40(%rbx)\n"
    "pop %rbx\n ret\n.size capture_filter,.-capture_filter\n");

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
    /* Turn the real loop's fallthrough into a return for isolated testing. */
    *(uint8_t *)UINT64_C(0x55182bf8d5) = 0xc3;
    unsigned state = 45678;
    for (int i = 0; i < 10000; i++) {
        uint8_t storage[258 * sizeof(float) + 3] __attribute__((aligned(16)));
        uint8_t *values = storage + (argc == 3 ? i % 4 : 0);
        int count = rand_r(&state) % 256 + 1;
        for (int k = 0; k <= count; k++) {
            float value = ((int)(rand_r(&state) % 200001) - 100000) / 123.0f;
            memcpy(values + k * sizeof(float), &value, sizeof(value));
        }
        uint64_t result[8];
        capture_filter((float *)(values + sizeof(float)),
                         (float *)(values + (count + 1) * sizeof(float)), result);
        if (fwrite(result, sizeof(result), 1, stdout) != 1 ||
            fwrite(values, sizeof(float), count + 1, stdout) != count + 1) {
            return 5;
        }
    }
    return 0;
}
