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

extern void capture_clear(unsigned char *, unsigned char *, uint64_t *,
                           const uint64_t *);
__asm__(
    ".text\n.global capture_clear\n.type capture_clear,@function\n"
    "capture_clear:\n push %rbx\n mov %rdx,%rbx\n"
    "movdqu (%rcx),%xmm0\n mov %rdi,%rdx\n mov %rsi,%rdi\n"
    "movabs $0x55090df340,%rax\n call *%rax\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n mov %rax,(%rbx)\n"
    "mov %rdx,8(%rbx)\n mov %rdi,16(%rbx)\n"
    "movdqu %xmm0,24(%rbx)\n pop %rbx\n ret\n"
    ".size capture_clear,.-capture_clear\n");

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)UINT64_C(0x5509024000);
    if (mmap(base, st.st_size, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    *(uint8_t *)UINT64_C(0x55090df358) = 0xc3;
    size_t page = sysconf(_SC_PAGESIZE);
    unsigned char *memory = mmap(NULL, page * 5, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED || mprotect(memory + page * 4, page, PROT_NONE)) {
        return 5;
    }
    unsigned state = 82923;
    for (int test = 0; test < 2000; test++) {
        size_t length = (rand_r(&state) % 128 + 1) * 64;
        unsigned char *start = memory + page * 4 - length;
        unsigned char *end = start + length - rand_r(&state) % 64;
        uint64_t pattern[2] = {rand_r(&state), rand_r(&state)};
        uint64_t captured[5];
        memset(start - 16, 0xa5, length + 16);
        capture_clear(start, end, captured, pattern);
        captured[1] -= (uintptr_t)start;
        captured[2] -= (uintptr_t)start;
        if (fwrite(captured, sizeof(captured), 1, stdout) != 1 ||
            fwrite(start - 16, length + 16, 1, stdout) != 1) return 6;
    }
    return 0;
}
