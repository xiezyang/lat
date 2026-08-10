/*
 * Verify that LATX AVX translation and guest AVX feature reporting can be
 * controlled independently. Build this file on an x86-64 host without global
 * -mavx flags; only execute_avx() is compiled for AVX.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cpuid_regs {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
};

static struct cpuid_regs read_cpuid(uint32_t leaf, uint32_t subleaf)
{
    struct cpuid_regs regs;

    __asm__ volatile("cpuid"
                     : "=a"(regs.eax), "=b"(regs.ebx),
                       "=c"(regs.ecx), "=d"(regs.edx)
                     : "a"(leaf), "c"(subleaf));
    return regs;
}

static uint64_t read_xcr0(void)
{
    uint32_t eax;
    uint32_t edx;

    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    return ((uint64_t)edx << 32) | eax;
}

extern unsigned int execute_vex_vpsrlq(void);

static unsigned int execute_avx(void)
{
    return execute_vex_vpsrlq();
}

__attribute__((noinline, target("bmi2")))
static unsigned int execute_bmi2(void)
{
    const unsigned int value = 0x3f;
    const unsigned int mask = 0x15;
    unsigned int output;

    __asm__ volatile("pext %2, %1, %0"
                     : "=r"(output)
                     : "r"(value), "r"(mask));
    return output;
}

static int bit_is_set(uint32_t value, unsigned int bit)
{
    return (value >> bit) & 1U;
}

static void print_cpuinfo_flags(void)
{
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    char *line = NULL;
    size_t size = 0;

    if (cpuinfo == NULL) {
        printf("cpuinfo_error=%s\n", strerror(errno));
        return;
    }

    while (getline(&line, &size, cpuinfo) >= 0) {
        if (strncmp(line, "flags", 5) == 0) {
            printf("cpuinfo_%s", line);
            break;
        }
    }

    free(line);
    fclose(cpuinfo);
}

static int avx_is_usable(struct cpuid_regs leaf1, uint64_t *xcr0)
{
    const int avx = bit_is_set(leaf1.ecx, 28);
    const int xsave = bit_is_set(leaf1.ecx, 26);
    const int osxsave = bit_is_set(leaf1.ecx, 27);

    if (!avx || !xsave || !osxsave) {
        *xcr0 = 0;
        return 0;
    }

    *xcr0 = read_xcr0();
    return (*xcr0 & 0x6) == 0x6;
}

static void print_features(struct cpuid_regs leaf0, struct cpuid_regs leaf1,
                           struct cpuid_regs leaf7,
                           struct cpuid_regs leafd0)
{
    printf("cpuid_max_basic=0x%x\n", leaf0.eax);
    printf("leaf1_ecx=0x%08x fma=%d xsave=%d osxsave=%d avx=%d f16c=%d\n",
           leaf1.ecx,
           bit_is_set(leaf1.ecx, 12),
           bit_is_set(leaf1.ecx, 26),
           bit_is_set(leaf1.ecx, 27),
           bit_is_set(leaf1.ecx, 28),
           bit_is_set(leaf1.ecx, 29));
    printf("leaf7_ebx=0x%08x bmi1=%d hle=%d avx2=%d bmi2=%d avx512f=%d\n",
           leaf7.ebx,
           bit_is_set(leaf7.ebx, 3),
           bit_is_set(leaf7.ebx, 4),
           bit_is_set(leaf7.ebx, 5),
           bit_is_set(leaf7.ebx, 8),
           bit_is_set(leaf7.ebx, 16));
    printf("leafd0_eax=0x%08x leafd0_ebx=0x%08x leafd0_ecx=0x%08x "
           "leafd0_edx=0x%08x\n",
           leafd0.eax, leafd0.ebx, leafd0.ecx, leafd0.edx);
    print_cpuinfo_flags();
}

static void usage(const char *program)
{
    fprintf(stderr, "usage: %s info|xgetbv|guarded|unconditional|bmi2\n",
            program);
}

int main(int argc, char **argv)
{
    struct cpuid_regs leaf0 = read_cpuid(0, 0);
    struct cpuid_regs leaf1 = read_cpuid(1, 0);
    struct cpuid_regs leaf7 = {0};
    struct cpuid_regs leafd0 = {0};
    uint64_t xcr0 = 0;

    if (argc != 2) {
        usage(argv[0]);
        return 2;
    }

    if (leaf0.eax >= 7) {
        leaf7 = read_cpuid(7, 0);
    }
    if (leaf0.eax >= 0xd) {
        leafd0 = read_cpuid(0xd, 0);
    }
    if (argv[1][0] == 'i') {
        print_features(leaf0, leaf1, leaf7, leafd0);
        return 0;
    }

    if (argv[1][0] == 'x') {
        printf("xcr0=0x%llx\n", (unsigned long long)read_xcr0());
        return 0;
    }

    if (argv[1][0] == 'g') {
        if (!avx_is_usable(leaf1, &xcr0)) {
            return 0;
        }
        return execute_avx() == 2 ? 0 : 1;
    }

    if (argv[1][0] == 'u') {
        return execute_avx() == 2 ? 0 : 1;
    }

    if (argv[1][0] == 'b') {
        const unsigned int result = execute_bmi2();

        return result == 7 ? 0 : 1;
    }

    usage(argv[0]);
    return 2;
}
