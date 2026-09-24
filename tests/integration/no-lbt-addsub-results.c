#include <stdint.h>
#include <stdio.h>

static uint64_t hash = UINT64_C(1469598103934665603);

static void record(uint64_t value)
{
    hash = (hash ^ value) * UINT64_C(1099511628211);
}

#define CHECK(OP, REG, INPUT) do { \
    uint64_t result = (INPUT), flags; \
    __asm__ volatile(OP " %" REG "2, %" REG "0; pushfq; popq %1" \
                     : "+&a"(result), "=r"(flags) : "r"(b) : "cc"); \
    record(result); \
    record(flags & 0x8d5); \
} while (0)

int main(void)
{
    uint64_t state = UINT64_C(0x123456789abcdef0);
    const uint64_t edges[] = {
        0, 1, 0x7f, 0x80, 0xff, 0x7fff, 0x8000, 0xffff,
        0x7fffffff, 0x80000000, 0xffffffff,
        UINT64_C(0x7fffffffffffffff), UINT64_C(0x8000000000000000),
        UINT64_MAX
    };
    for (unsigned int i = 0; i < 20000; i++) {
        state = state * UINT64_C(6364136223846793005) + 1;
        uint64_t a = i < 196 ? edges[i / 14] : state;
        state = state * UINT64_C(6364136223846793005) + 1;
        uint64_t b = i < 196 ? edges[i % 14] : state;
        CHECK("addb", "b", a);
        CHECK("subb", "b", a);
        CHECK("addw", "w", a);
        CHECK("subw", "w", a);
        CHECK("addl", "k", a);
        CHECK("subl", "k", a);
        CHECK("addq", "q", a);
        CHECK("subq", "q", a);
    }
    printf("%016llx\n", (unsigned long long)hash);
    return 0;
}
