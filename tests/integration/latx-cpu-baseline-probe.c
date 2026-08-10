/* Dump guest-visible CPU state for exact comparison between LATX builds. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

static unsigned int bit_is_set(uint32_t value, unsigned int bit)
{
    return (value >> bit) & 1U;
}

static void print_cpuid(const char *kind, uint32_t leaf, uint32_t subleaf)
{
    struct cpuid_regs regs = read_cpuid(leaf, subleaf);

    printf("%s leaf=%08x subleaf=%08x eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
           kind, leaf, subleaf, regs.eax, regs.ebx, regs.ecx, regs.edx);
}

static void print_bounded_leaf_range(uint32_t first, uint32_t last)
{
    uint32_t leaf;

    if (last < first || last - first > 0x100U) {
        return;
    }
    for (leaf = first; leaf <= last; leaf++) {
        print_cpuid("CPUID", leaf, 0);
    }
}

static void print_indexed_leaf(uint32_t leaf, uint32_t last_subleaf)
{
    uint32_t subleaf;

    if (last_subleaf > 0x100U) {
        last_subleaf = 0x100U;
    }
    for (subleaf = 0; subleaf <= last_subleaf; subleaf++) {
        print_cpuid("CPUID_SUBLEAF", leaf, subleaf);
    }
}

static void print_topology_leaf(uint32_t leaf)
{
    uint32_t subleaf;

    for (subleaf = 0; subleaf < 32; subleaf++) {
        struct cpuid_regs regs = read_cpuid(leaf, subleaf);

        printf("CPUID_SUBLEAF leaf=%08x subleaf=%08x eax=%08x ebx=%08x "
               "ecx=%08x edx=%08x\n",
               leaf, subleaf, regs.eax, regs.ebx, regs.ecx, regs.edx);
        if (regs.ebx == 0) {
            break;
        }
    }
}

static void print_deterministic_cache_leaf(void)
{
    uint32_t subleaf;

    for (subleaf = 0; subleaf < 32; subleaf++) {
        struct cpuid_regs regs = read_cpuid(4, subleaf);

        printf("CPUID_SUBLEAF leaf=00000004 subleaf=%08x eax=%08x ebx=%08x "
               "ecx=%08x edx=%08x\n",
               subleaf, regs.eax, regs.ebx, regs.ecx, regs.edx);
        if ((regs.eax & 0x1fU) == 0) {
            break;
        }
    }
}

static void print_cpuinfo(void)
{
    FILE *file = fopen("/proc/cpuinfo", "r");
    char buffer[4096];
    size_t size;

    if (file == NULL) {
        perror("/proc/cpuinfo");
        exit(1);
    }

    puts("CPUINFO_BEGIN");
    while ((size = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        if (fwrite(buffer, 1, size, stdout) != size) {
            perror("stdout");
            fclose(file);
            exit(1);
        }
    }
    if (ferror(file)) {
        perror("/proc/cpuinfo");
        fclose(file);
        exit(1);
    }
    fclose(file);
    putchar('\n');
    puts("CPUINFO_END");
}

int main(void)
{
    struct cpuid_regs leaf0 = read_cpuid(0, 0);
    struct cpuid_regs leaf1 = read_cpuid(1, 0);
    struct cpuid_regs leaf7 = {0};
    struct cpuid_regs ext0 = read_cpuid(0x80000000U, 0);
    unsigned int osxsave = bit_is_set(leaf1.ecx, 27);

    if (leaf0.eax >= 7) {
        leaf7 = read_cpuid(7, 0);
    }

    puts("LATX_CPU_BASELINE_V1");
    printf("FEATURES fma=%u xsave=%u osxsave=%u avx=%u f16c=%u avx2=%u\n",
           bit_is_set(leaf1.ecx, 12), bit_is_set(leaf1.ecx, 26), osxsave,
           bit_is_set(leaf1.ecx, 28), bit_is_set(leaf1.ecx, 29),
           bit_is_set(leaf7.ebx, 5));
    if (osxsave) {
        printf("XGETBV visible=1 xcr0=%016llx\n",
               (unsigned long long)read_xcr0());
    } else {
        puts("XGETBV visible=0");
    }

    print_bounded_leaf_range(0, leaf0.eax);
    print_bounded_leaf_range(0x80000000U, ext0.eax);

    if (bit_is_set(leaf1.ecx, 31)) {
        struct cpuid_regs hypervisor = read_cpuid(0x40000000U, 0);

        print_bounded_leaf_range(0x40000000U, hypervisor.eax);
    }
    if (leaf0.eax >= 4) {
        print_deterministic_cache_leaf();
    }
    if (leaf0.eax >= 7) {
        print_indexed_leaf(7, leaf7.eax);
    }
    if (leaf0.eax >= 0xb) {
        print_topology_leaf(0xb);
    }
    if (leaf0.eax >= 0xd) {
        print_indexed_leaf(0xd, 63);
    }
    if (leaf0.eax >= 0x14) {
        struct cpuid_regs regs = read_cpuid(0x14, 0);

        print_indexed_leaf(0x14, regs.eax);
    }
    if (leaf0.eax >= 0x17) {
        struct cpuid_regs regs = read_cpuid(0x17, 0);

        print_indexed_leaf(0x17, regs.eax);
    }
    if (leaf0.eax >= 0x18) {
        struct cpuid_regs regs = read_cpuid(0x18, 0);

        print_indexed_leaf(0x18, regs.eax);
    }
    if (leaf0.eax >= 0x1f) {
        print_topology_leaf(0x1f);
    }

    print_cpuinfo();
    return 0;
}
