/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !defined(__loongarch64)
int main(void)
{
    puts("SKIP: run this probe natively on LoongArch64 Linux");
    return 77;
}
#else
/* Emit v1.1 AM encodings even with assemblers that only know the base ISA.
 * $a0/$a1/$a2 are rd/address/source, respectively. Never emulate a fault.
 */
#define STRING_(x) #x
#define STRING(x) STRING_(x)
#define AM(name, opcode) \
static uint64_t name(void *p, uint64_t source, uint64_t expected) \
{ \
    register uint64_t rd __asm__("$a0") = expected; \
    register uintptr_t address __asm__("$a1") = (uintptr_t)p; \
    register uint64_t value __asm__("$a2") = source; \
    __asm__ volatile(".word (" STRING(opcode) ") | (6 << 10) | (5 << 5) | 4" \
                     : "+r"(rd) : "r"(address), "r"(value) : "memory"); \
    return rd; \
}
AM(add_b, 0x385f0000) AM(add_h, 0x385f8000)
AM(add_w, 0x386a0000) AM(add_d, 0x386a8000)
AM(swap_b, 0x385e0000) AM(swap_h, 0x385e8000)
AM(swap_w, 0x38690000) AM(swap_d, 0x38698000)
AM(cas_b, 0x385a0000) AM(cas_h, 0x385a8000)
AM(cas_w, 0x385b0000) AM(cas_d, 0x385b8000)
AM(and_w, 0x386b0000) AM(and_d, 0x386b8000)
AM(or_w, 0x386c0000) AM(or_d, 0x386c8000)
AM(xor_w, 0x386d0000) AM(xor_d, 0x386d8000)

#define LLSC(name, suffix) \
static uint64_t name(void *p, uint64_t source, uint64_t expected) \
{ \
    uint64_t old, success; \
    (void)expected; \
    __asm__ volatile("dbar 0" ::: "memory"); \
    for (unsigned n = 0; n < 1000000; n++) { \
        __asm__ volatile("ll." suffix " %0, %2, 0\n\t" \
                         "add.d %1, %0, %3\n\t" \
                         "sc." suffix " %1, %2, 0" \
                         : "=&r"(old), "=&r"(success) \
                         : "r"(p), "r"(source) : "memory"); \
        if (success) { \
            __asm__ volatile("dbar 0" ::: "memory"); \
            return old; \
        } \
    } \
    fputs("FAIL: LL/SC retry limit\n", stderr); \
    exit(1); \
}
LLSC(llsc_w, "w") LLSC(llsc_d, "d")

typedef uint64_t (*operation)(void *, uint64_t, uint64_t);
enum kind { ADD, SWAP, CAS, AND, OR, XOR };
struct probe { const char *name; operation fn; int bytes; enum kind kind; };
static const struct probe probes[] = {
    {"amadd_db.b", add_b, 1, ADD}, {"amadd_db.h", add_h, 2, ADD},
    {"amadd_db.w", add_w, 4, ADD}, {"amadd_db.d", add_d, 8, ADD},
    {"amswap_db.b", swap_b, 1, SWAP}, {"amswap_db.h", swap_h, 2, SWAP},
    {"amswap_db.w", swap_w, 4, SWAP}, {"amswap_db.d", swap_d, 8, SWAP},
    {"amcas_db.b", cas_b, 1, CAS}, {"amcas_db.h", cas_h, 2, CAS},
    {"amcas_db.w", cas_w, 4, CAS}, {"amcas_db.d", cas_d, 8, CAS},
    {"amand_db.w", and_w, 4, AND}, {"amand_db.d", and_d, 8, AND},
    {"amor_db.w", or_w, 4, OR}, {"amor_db.d", or_d, 8, OR},
    {"amxor_db.w", xor_w, 4, XOR}, {"amxor_db.d", xor_d, 8, XOR},
    {"ll.w/sc.w", llsc_w, 4, ADD}, {"ll.d/sc.d", llsc_d, 8, ADD},
};
static sigjmp_buf recovery;
static volatile sig_atomic_t armed, caught_signal, caught_code;
static unsigned char arena[192] __attribute__((aligned(64)));

static void fault(int sig, siginfo_t *info, void *context)
{
    (void)context;
    if (!armed) {
        _exit(128 + sig);
    }
    caught_signal = sig;
    caught_code = info->si_code;
    siglongjmp(recovery, 1);
}

static int attempt(operation fn, void *p, uint64_t src, uint64_t cmp, uint64_t *old)
{
    caught_signal = caught_code = 0;
    if (sigsetjmp(recovery, 1)) {
        armed = 0;
        return -1;
    }
    armed = 1;
    *old = fn(p, src, cmp);
    armed = 0;
    return 0;
}

static uint64_t mask(int bytes)
{
    return bytes == 8 ? UINT64_MAX : (UINT64_C(1) << (bytes * 8)) - 1;
}

static int sweep(const struct probe *p)
{
    unsigned char saved[sizeof(arena)];
    unsigned cases = 0;
    for (int offset = 0; offset <= 64 - p->bytes; offset++) {
        for (int mismatch = 0; mismatch <= (p->kind == CAS); mismatch++) {
            uint64_t initial = mask(p->bytes), src = 0x123456789abcdef1ULL, old = 0, value = 0;
            void *address = arena + 64 + offset;
            memset(arena, 0xa5, sizeof(arena));
            memcpy(address, &initial, p->bytes);
            memcpy(saved, arena, sizeof(arena));
            if (attempt(p->fn, address, src, initial - mismatch, &old)) {
                printf("UNSUPPORTED: %s offset=%d signal=%d si_code=%d\n",
                       p->name, offset, caught_signal, caught_code);
                return 77;
            }
            memcpy(&value, address, p->bytes);
            uint64_t expected = p->kind == ADD ? initial + src :
                p->kind == SWAP ? src : p->kind == CAS ? (mismatch ? initial : src) :
                p->kind == AND ? initial & src : p->kind == OR ? initial | src : initial ^ src;
            if ((old & mask(p->bytes)) != initial || value != (expected & mask(p->bytes))) {
                fprintf(stderr, "FAIL: %s offset=%d old=%" PRIx64 " value=%" PRIx64 "\n",
                        p->name, offset, old, value);
                return 1;
            }
            for (unsigned n = 0; n < sizeof(arena); n++) {
                if ((n < 64u + offset || n >= 64u + offset + p->bytes) && arena[n] != saved[n]) {
                    fprintf(stderr, "FAIL: %s changed adjacent byte %u\n", p->name, n);
                    return 1;
                }
            }
            cases++;
        }
    }
    printf("PASS: %s %u cache-line-local cases, no fault or adjacent-byte change\n", p->name, cases);
    return 0;
}

struct worker {
    const struct probe *probe;
    void *address;
    int cpu;
    uint32_t tickets[256];
    int failed;
};
static pthread_barrier_t start;
#define ITERATIONS 10000
static void *contend(void *opaque)
{
    struct worker *w = opaque;
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(w->cpu, &cpus);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpus), &cpus)) {
        w->failed = 1;
    }
    int r = pthread_barrier_wait(&start);
    if (r && r != PTHREAD_BARRIER_SERIAL_THREAD) {
        w->failed = 1;
    }
    for (int n = 0; n < ITERATIONS; n++) {
        uint64_t old;
        if (w->probe->kind == CAS) {
            uint64_t expected = 0;
            for (unsigned tries = 0;; tries++) {
                old = w->probe->fn(w->address, expected + 1, expected);
                old &= mask(w->probe->bytes);
                if (old == expected) {
                    break;
                }
                if (tries == 1000000) {
                    w->failed = 1;
                    return NULL;
                }
                expected = old;
            }
        } else {
            old = w->probe->fn(w->address, 1, 0);
        }
        w->tickets[old & 255]++;
    }
    return NULL;
}

static int contention(const struct probe *p)
{
    if (p->kind != ADD && p->kind != CAS) {
        return 0;
    }
    cpu_set_t available;
    int selected[2], found = 0;
    if (sched_getaffinity(0, sizeof(available), &available)) {
        perror("sched_getaffinity");
        return 1;
    }
    for (int n = 0; n < CPU_SETSIZE && found < 2; n++) {
        if (CPU_ISSET(n, &available)) {
            selected[found++] = n;
        }
    }
    if (found < 2) {
        puts("SKIP: concurrent atomicity requires two available CPUs");
        return 77;
    }
    struct worker workers[2] = {{.probe = p}, {.probe = p}};
    pthread_t threads[2];
    unsigned char saved[sizeof(arena)];
    /* Exercise every non-natural alignment, including crossing an 8-byte word. */
    for (int offset = 0; offset < p->bytes; offset++) {
        memset(arena, 0xa5, sizeof(arena));
        memset(arena + 64 + offset, 0, p->bytes);
        memcpy(saved, arena, sizeof(arena));
        if (pthread_barrier_init(&start, NULL, 2)) {
            return 1;
        }
        for (int n = 0; n < 2; n++) {
            workers[n].address = arena + 64 + offset;
            workers[n].cpu = selected[n];
            workers[n].failed = 0;
            memset(workers[n].tickets, 0, sizeof(workers[n].tickets));
            if (pthread_create(&threads[n], NULL, contend, &workers[n])) {
                fputs("FAIL: pthread_create\n", stderr);
                exit(1);
            }
        }
        for (int n = 0; n < 2; n++) {
            if (pthread_join(threads[n], NULL) || workers[n].failed) {
                return 1;
            }
        }
        pthread_barrier_destroy(&start);
        uint64_t value = 0;
        memcpy(&value, arena + 64 + offset, p->bytes);
        if (value != ((2 * ITERATIONS) & mask(p->bytes))) {
            fprintf(stderr, "FAIL: %s lost updates at offset %d\n", p->name, offset);
            return 1;
        }
        for (int n = 0; n < 256; n++) {
            unsigned expected = (2 * ITERATIONS) / 256 + (n < (2 * ITERATIONS) % 256);
            if (workers[0].tickets[n] + workers[1].tickets[n] != expected) {
                fprintf(stderr, "FAIL: %s returned duplicate/missing old values\n", p->name);
                return 1;
            }
        }
        for (unsigned n = 0; n < sizeof(arena); n++) {
            if ((n < 64u + offset || n >= 64u + offset + p->bytes) && arena[n] != saved[n]) {
                fputs("FAIL: contention changed adjacent bytes\n", stderr);
                return 1;
            }
        }
    }
    printf("PASS: %s contention on CPUs %d/%d, all natural-alignment residues\n",
           p->name, selected[0], selected[1]);
    return 0;
}

int main(int argc, char **argv)
{
    int require = argc == 2 && !strcmp(argv[1], "--require");
    if (argc > 2 || (argc == 2 && !require)) {
        fprintf(stderr, "usage: %s [--require]\n", argv[0]);
        return 2;
    }
    struct sigaction sa = {.sa_sigaction = fault, .sa_flags = SA_SIGINFO};
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGBUS, &sa, NULL) || sigaction(SIGSEGV, &sa, NULL) || sigaction(SIGILL, &sa, NULL)) {
        perror("sigaction");
        return 1;
    }
    unsigned cfg2;
    __asm__ volatile("cpucfg %0, %1" : "=r"(cfg2) : "r"(2));
    printf("CPUCFG.2=%08x; probe uses raw host instructions, no LAT or fault emulation\n", cfg2);
    for (unsigned n = 0; n < sizeof(probes) / sizeof(probes[0]); n++) {
        int r = sweep(&probes[n]);
        if (!r) {
            r = contention(&probes[n]);
        }
        if (r) {
            return require && r == 77 ? 1 : r;
        }
    }
    puts("PASS: cache-line-local unaligned atomics and pinned two-CPU contention");
    return 0;
}
#endif
