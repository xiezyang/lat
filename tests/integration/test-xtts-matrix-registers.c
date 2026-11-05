/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct frame {
    uint64_t args[6], stack[5], entry, result[56];
};
extern void capture_matrix(struct frame *);
__asm__(
    ".text\n.global capture_matrix\n.type capture_matrix,@function\n"
    "capture_matrix:\n push %rbx\n push %rbp\n push %r12\n push %r13\n"
    "push %r14\n push %r15\n sub $72,%rsp\n mov %rdi,48(%rsp)\n"
    "mov %rdi,%r10\n mov 88(%r10),%rax\n mov %rax,56(%rsp)\n"
    "mov 48(%r10),%rax\n mov %rax,0(%rsp)\n"
    "mov 56(%r10),%rax\n mov %rax,8(%rsp)\n"
    "mov 64(%r10),%rax\n mov %rax,16(%rsp)\n"
    "mov 72(%r10),%rax\n mov %rax,24(%rsp)\n"
    "mov 80(%r10),%rax\n mov %rax,32(%rsp)\n"
    "pcmpeqb %xmm0,%xmm0\n movdqa %xmm0,%xmm1\n"
    "movdqa %xmm0,%xmm2\n movdqa %xmm0,%xmm3\n"
    "movdqa %xmm0,%xmm4\n movdqa %xmm0,%xmm5\n"
    "movdqa %xmm0,%xmm6\n movdqa %xmm0,%xmm7\n"
    "movdqa %xmm0,%xmm8\n movdqa %xmm0,%xmm9\n"
    "movdqa %xmm0,%xmm10\n movdqa %xmm0,%xmm11\n"
    "movdqa %xmm0,%xmm12\n movdqa %xmm0,%xmm13\n"
    "movdqa %xmm0,%xmm14\n movdqa %xmm0,%xmm15\n"
    "mov 0(%r10),%rdi\n mov 8(%r10),%rsi\n mov 16(%r10),%rdx\n"
    "mov 24(%r10),%rcx\n mov 32(%r10),%r8\n mov 40(%r10),%r9\n"
    "mov $0x12345678,%eax\n mov $0x23456789,%ebx\n"
    "mov $0x3456789a,%ebp\n mov $0x456789ab,%r12d\n"
    "mov $0x56789abc,%r13d\n mov $0x6789abcd,%r14d\n"
    "mov $0x789abcde,%r15d\n mov $0x89abcdef,%r10d\n"
    "mov $0x9abcdef0,%r11d\n"
    "movq %rax,%mm0\n movq %rax,%mm1\n movq %rax,%mm2\n movq %rax,%mm3\n"
    "movq %rax,%mm4\n movq %rax,%mm5\n movq %rax,%mm6\n movq %rax,%mm7\n"
    "call *56(%rsp)\n"
    "mov %r10,40(%rsp)\n mov 48(%rsp),%r10\n lea 96(%r10),%r10\n"
    "mov %rax,0(%r10)\n mov %rcx,8(%r10)\n mov %rdx,16(%r10)\n"
    "mov %rbx,24(%r10)\n mov %rbp,32(%r10)\n mov %rsi,40(%r10)\n"
    "mov %rdi,48(%r10)\n mov %r8,56(%r10)\n mov %r9,64(%r10)\n"
    "mov %r11,80(%r10)\n mov %r12,88(%r10)\n mov %r13,96(%r10)\n"
    "mov %r14,104(%r10)\n mov %r15,112(%r10)\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n mov %rax,120(%r10)\n"
    "mov 40(%rsp),%rax\n mov %rax,72(%r10)\n"
    "movdqu %xmm0,128(%r10)\n movdqu %xmm1,144(%r10)\n"
    "movdqu %xmm2,160(%r10)\n movdqu %xmm3,176(%r10)\n"
    "movdqu %xmm4,192(%r10)\n movdqu %xmm5,208(%r10)\n"
    "movdqu %xmm6,224(%r10)\n movdqu %xmm7,240(%r10)\n"
    "movdqu %xmm8,256(%r10)\n movdqu %xmm9,272(%r10)\n"
    "movdqu %xmm10,288(%r10)\n movdqu %xmm11,304(%r10)\n"
    "movdqu %xmm12,320(%r10)\n movdqu %xmm13,336(%r10)\n"
    "movdqu %xmm14,352(%r10)\n movdqu %xmm15,368(%r10)\n"
    "movq %mm0,384(%r10)\n movq %mm1,392(%r10)\n"
    "movq %mm2,400(%r10)\n movq %mm3,408(%r10)\n"
    "movq %mm4,416(%r10)\n movq %mm5,424(%r10)\n"
    "movq %mm6,432(%r10)\n movq %mm7,440(%r10)\n emms\n"
    "add $72,%rsp\n pop %r15\n pop %r14\n pop %r13\n pop %r12\n"
    "pop %rbp\n pop %rbx\n ret\n.size capture_matrix,.-capture_matrix\n");

int main(int argc, char **argv)
{
    struct stat st;
    if (argc != 2) return 2;
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)UINT64_C(0x5518000000);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    uint8_t *arena = mmap((void *)UINT64_C(0x6000000000), 16384,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                          -1, 0);
    if (arena == MAP_FAILED) return 5;
    const uint64_t entries[] = {0x19c200, 0x19c4a0, 0x1989b0, 0x1992e0,
                                0x19b9c0, 0x198d50, 0x19d8e0};
    struct frame f;
    for (unsigned k = 0; k < 7; k++) {
        for (int i = 0; i < 16384; i++) arena[i] = i % 16;
        memset(&f, 0, sizeof(f));
        f.args[0] = 64; f.args[1] = 8;
        f.entry = (uintptr_t)base + entries[k];
        f.args[3] = (uintptr_t)arena;
        f.args[4] = (uintptr_t)(arena + 512);
        f.args[5] = (uintptr_t)(arena + 4096);
        f.stack[0] = (uintptr_t)(arena + 8192);
        f.stack[1] = 4; f.stack[2] = 6; f.stack[3] = 8; f.stack[4] = 9;
        if (k == 2 || k == 3) {
            f.args[2] = (uintptr_t)arena;
            f.args[3] = (uintptr_t)(arena + 512);
            f.args[4] = (uintptr_t)(arena + 4096);
            f.args[5] = (uintptr_t)(arena + 8192);
            f.stack[0] = 4; f.stack[1] = 6; f.stack[2] = 8; f.stack[3] = 9;
        } else if (k == 5) {
            memset(arena, 0, 512);
            f.stack[1] = (uintptr_t)(arena + 10240);
        } else if (k == 6) {
            f.args[2] = 3;
        }
        capture_matrix(&f);
        if (fwrite(f.result, sizeof(f.result), 1, stdout) != 1 ||
            fwrite(arena, 16384, 1, stdout) != 1) return 6;
    }
    return 0;
}
