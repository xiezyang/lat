/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <link.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <ucontext.h>

struct mpi { int sign; size_t count; uint64_t *limbs; };
typedef int (*copy_fn)(struct mpi *, const struct mpi *);
typedef size_t (*bitlen_fn)(const struct mpi *);
typedef int (*shift_fn)(struct mpi *, size_t);
typedef int (*cmp_fn)(const struct mpi *, const struct mpi *);

static uint64_t next(uint64_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

static int emit(const void *data, size_t size)
{
    return fwrite(data, size, 1, stdout) == 1 ? 0 : 1;
}

static int env_count(const char *name)
{
    const char *value = getenv(name);
    if (!value || !*value) return 20000;
    return atoi(value);
}

static void fault_handler(int signal, siginfo_t *info, void *context)
{
    ucontext_t *uc = context;
    dprintf(2, "GUEST_FAULT signal=%d addr=%p rip=%p\n", signal, info->si_addr,
            (void *)uc->uc_mcontext.gregs[REG_RIP]);
    _exit(128 + signal);
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    struct sigaction action = { .sa_sigaction = fault_handler,
                                .sa_flags = SA_SIGINFO };
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
    void *handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    struct link_map *lm;
    if (!handle || dlinfo(handle, RTLD_DI_LINKMAP, &lm) != 0) return 3;
    uintptr_t image = (uintptr_t)lm->l_addr;
    uint64_t *arena = mmap((void *)UINT64_C(0x6000000000), 65536,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                           -1, 0);
    if (arena == MAP_FAILED) return 5;
    copy_fn copy = (copy_fn)(image + UINT64_C(0x26897f));
    bitlen_fn bitlen = (bitlen_fn)(image + UINT64_C(0x2690d5));
    shift_fn shift_l = (shift_fn)(image + UINT64_C(0x269f1b));
    shift_fn shift_r = (shift_fn)(image + UINT64_C(0x26a11a));
    cmp_fn cmp_abs = (cmp_fn)(image + UINT64_C(0x26a2f6));
    int copy_count = env_count("LATX_TEST_COPY_COUNT");
    int bitlen_count = env_count("LATX_TEST_BITLEN_COUNT");
    int shift_l_count = env_count("LATX_TEST_SHIFT_L_COUNT");
    int shift_r_count = env_count("LATX_TEST_SHIFT_R_COUNT");
    int cmp_count = env_count("LATX_TEST_CMP_COUNT");

    uint64_t seed = 0x243f6a8885a308d3ULL;
    for (int t = 0; t < copy_count; t++) {
        struct mpi x = { t & 1 ? -1 : 1, 64, arena };
        struct mpi y = { t & 2 ? -1 : 1, 1 + (t % 64), arena + 80 };
        for (size_t i = 0; i < 64; i++) {
            x.limbs[i] = next(&seed);
            y.limbs[i] = i < y.count ? next(&seed) : 0;
        }
        if (t % 7 == 0) y.limbs[y.count - 1] = 0;
        for (size_t i = 0; i < 64; i++) x.limbs[i] = next(&seed);
        int ret = copy(&x, &y);
        if (emit(&ret, sizeof(ret)) || emit(&x.sign, sizeof(x.sign)) ||
            emit(x.limbs, 64 * sizeof(*x.limbs))) return 6;
    }

    seed = 0x13198a2e03707344ULL;
    for (int t = 0; t < bitlen_count; t++) {
        struct mpi x = { t & 1 ? -1 : 1, 64, arena };
        for (size_t i = 0; i < 64; i++)
            x.limbs[i] = (t % 11 == 0 && i > (size_t)(t % 64)) ? 0 : next(&seed);
        size_t ret = bitlen(&x);
        if (emit(&ret, sizeof(ret))) return 7;
    }

    seed = 0xa4093822299f31d0ULL;
    for (int t = 0; t < shift_l_count; t++) {
        struct mpi x = { t & 1 ? -1 : 1, 64, arena };
        for (size_t i = 0; i < 64; i++)
            x.limbs[i] = i < 8 ? next(&seed) : 0;
        int ret = shift_l(&x, t % 2049);
        if (emit(&ret, sizeof(ret)) || emit(&x.sign, sizeof(x.sign)) ||
            emit(x.limbs, 64 * sizeof(*x.limbs))) return 8;
    }

    seed = 0x082efa98ec4e6c89ULL;
    for (int t = 0; t < shift_r_count; t++) {
        struct mpi x = { t & 1 ? -1 : 1, 64, arena };
        for (size_t i = 0; i < 64; i++) x.limbs[i] = next(&seed);
        int ret = shift_r(&x, t % 601);
        if (emit(&ret, sizeof(ret)) || emit(&x.sign, sizeof(x.sign)) ||
            emit(x.limbs, 64 * sizeof(*x.limbs))) return 9;
    }

    seed = 0x452821e638d01377ULL;
    for (int t = 0; t < cmp_count; t++) {
        struct mpi x = { t & 1 ? -1 : 1, 64, arena };
        struct mpi y = { t & 2 ? -1 : 1, 64, arena + 80 };
        for (size_t i = 0; i < 64; i++) {
            x.limbs[i] = (t % 13 == 0 && i > (size_t)(t % 64)) ? 0 : next(&seed);
            y.limbs[i] = (t % 17 == 0 && i > (size_t)((t * 3) % 64)) ? 0 : next(&seed);
        }
        int ret = cmp_abs(&x, &y);
        if (emit(&ret, sizeof(ret))) return 10;
    }
    return 0;
}
