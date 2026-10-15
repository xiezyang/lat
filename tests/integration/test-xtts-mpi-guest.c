/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern void capture_mpi(uint64_t, const uint64_t *, uint64_t *, uint64_t,
                         uint64_t *);
extern void capture_sub(uint64_t, const uint64_t *, uint64_t *, uint64_t,
                         uint64_t *);
extern void capture_add32(uint64_t, const uint64_t *, uint64_t *, uint64_t,
                           uint64_t *);
extern void capture_sub32(uint64_t, const uint64_t *, uint64_t *, uint64_t,
                           uint64_t *);
__asm__(
    ".text\n.global capture_add32\n.type capture_add32,@function\n"
    "capture_add32:\n"
    "movabs $0x55084ba310,%rax\n jmp capture_common\n"
    ".size capture_add32,.-capture_add32\n"
    ".global capture_sub32\n.type capture_sub32,@function\n"
    "capture_sub32:\n"
    "movabs $0x55084ba351,%rax\n jmp capture_common\n"
    ".size capture_sub32,.-capture_sub32\n"
    ".text\n.global capture_sub\n.type capture_sub,@function\n"
    "capture_sub:\n"
    "movabs $0x550846aa00,%rax\n"
    "jmp capture_common\n"
    ".size capture_sub,.-capture_sub\n"
    ".text\n.global capture_mpi\n.type capture_mpi,@function\n"
    "capture_mpi:\n"
    "movabs $0x550846ae71,%rax\n"
    "capture_common:\n"
    "push %r12\n"
    "mov %r8,%r12\n"
    "call *%rax\n"
    "mov %rax,0(%r12)\n"
    "mov %rdx,8(%r12)\n"
    "mov %rcx,16(%r12)\n"
    "mov %rsi,24(%r12)\n"
    "mov %rdi,32(%r12)\n"
    "mov %r8,40(%r12)\n"
    "mov %r9,48(%r12)\n"
    "pushfq\n pop %rax\n and $0x8d5,%eax\n"
    "mov %rax,56(%r12)\n"
    "pop %r12\n ret\n.size capture_mpi,.-capture_mpi\n");

static uint64_t next(uint64_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) return 2;
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)UINT64_C(0x5508200000);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    uint64_t *arena = mmap((void *)UINT64_C(0x6000000000), 16384,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                            -1, 0);
    if (arena == MAP_FAILED) return 5;
    uint64_t state = 43219;
    for (int test = 0; test < 20000; test++) {
        unsigned n = test % 129;
        for (unsigned i = 0; i < 2048; i++) {
            arena[i] = test % 7 == 0 ? UINT64_MAX : next(&state);
        }
        /* Terminate carry propagation beyond the valid n-limb output. */
        unsigned src = 128, dst = test % 3 == 0 ? 128 : 512;
        if (test % 5 == 0) dst = 129;
        arena[dst + n + 1] = argc == 3 ? UINT64_MAX : 0;
        uint64_t result[8];
        uint64_t factor = test % 11 == 0 ? UINT64_MAX : next(&state);
        if (argc == 3 && (!strcmp(argv[2], "add32") ||
                          !strcmp(argv[2], "sub32"))) {
            uint64_t *carry = arena + dst + (test % 3 == 0 ? 0 : 256);
            if (!strcmp(argv[2], "add32")) {
                capture_add32((uintptr_t)(arena + dst),
                                (void *)(uintptr_t)(uint32_t)factor,
                                carry, factor, result);
            } else {
                capture_sub32((uintptr_t)(arena + dst),
                                (void *)(uintptr_t)(uint32_t)factor,
                                carry, factor, result);
            }
        } else if (argc == 3) {
            capture_sub(n, arena + src, arena + dst, factor, result);
        } else {
            capture_mpi(n, arena + src, arena + dst, factor, result);
        }
        /* R8 initially points to this host-stack capture buffer when n=0. */
        if (!n || argc == 3) result[5] = 0;
        if (argc == 3) result[6] = 0;
        uint64_t hash = UINT64_C(0xcbf29ce484222325);
        for (unsigned i = 0; i < 2048; i++) {
            hash = (hash ^ arena[i]) * UINT64_C(0x100000001b3);
        }
        if (fwrite(result, sizeof(result), 1, stdout) != 1 ||
            fwrite(&hash, sizeof(hash), 1, stdout) != 1) return 6;
    }
    return 0;
}
