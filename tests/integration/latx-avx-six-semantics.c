/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void avx_vmovdqa_ymm(const void *, void *);
void avx_vmovdqa_xmm(const void *, void *, void *);
void avx_vmovdqu_ymm(const void *, void *);
void avx_vmovdqu_xmm(const void *, void *, void *);
void avx_vpxor_ymm_reg(const void *, const void *, void *);
void avx_vpxor_ymm_mem_overlap(const void *, const void *, void *);
void avx_vpxor_xmm_reg(const void *, const void *, void *);
void avx_vpxor_xmm_mem(const void *, const void *, void *);
void avx_vextracti128(const void *, void *);
void avx_vzeroupper(const void *, void *);
void avx_vmovss_three(const void *, const void *, void *);
void avx_vmovss_load(const void *, const void *, void *);
void avx_vmovss_store(const void *, void *);
void avx_cross_tb(const void *, void *);
void avx_across_syscall(const void *, void *);

static int failures;

static void check_bytes(const char *name, const void *actual,
                        const void *expected, size_t size)
{
    if (memcmp(actual, expected, size) == 0) {
        printf("PASS %s\n", name);
        return;
    }

    const uint8_t *a = actual;
    const uint8_t *e = expected;

    failures++;
    for (size_t i = 0; i < size; ++i) {
        if (a[i] != e[i]) {
            printf("FAIL %s byte=%zu actual=%02x expected=%02x\n",
                   name, i, a[i], e[i]);
            return;
        }
    }
}

static void fill_inputs(uint8_t *a, uint8_t *b, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        a[i] = (uint8_t)(0x11 + i * 3);
        b[i] = (uint8_t)(0xe7 - i * 5);
    }
}

int main(void)
{
    _Alignas(32) uint8_t a[96];
    _Alignas(32) uint8_t b[96];
    _Alignas(32) uint8_t actual[128];
    _Alignas(32) uint8_t expected[128];

    fill_inputs(a, b, sizeof(a));

    memset(actual, 0xa5, sizeof(actual));
    avx_vmovdqa_ymm(a, actual);
    check_bytes("vmovdqa ymm mem/reg/store", actual, a, 32);

    memset(actual, 0xa5, sizeof(actual));
    memset(expected, 0, 32);
    memcpy(expected, a, 16);
    avx_vmovdqa_xmm(a, actual, actual + 32);
    check_bytes("vmovdqa xmm memory store", actual, a, 16);
    check_bytes("vmovdqa xmm clears high", actual + 32, expected, 32);

    memset(actual, 0xa5, sizeof(actual));
    avx_vmovdqu_ymm(a + 1, actual + 3);
    check_bytes("vmovdqu ymm unaligned", actual + 3, a + 1, 32);

    memset(actual, 0xa5, sizeof(actual));
    avx_vmovdqu_xmm(a + 1, actual + 3, actual + 35);
    memset(expected, 0, 32);
    memcpy(expected, a + 1, 16);
    check_bytes("vmovdqu xmm unaligned store", actual + 3, a + 1, 16);
    check_bytes("vmovdqu xmm clears high", actual + 35, expected, 32);

    for (size_t i = 0; i < 32; ++i) {
        expected[i] = a[i] ^ b[i];
    }
    avx_vpxor_ymm_reg(a, b, actual);
    check_bytes("vpxor ymm register", actual, expected, 32);
    avx_vpxor_ymm_mem_overlap(a, b, actual);
    check_bytes("vpxor ymm memory overlap", actual, expected, 32);

    memset(expected + 16, 0, 16);
    avx_vpxor_xmm_reg(a, b, actual);
    check_bytes("vpxor xmm register clears high", actual, expected, 32);
    avx_vpxor_xmm_mem(a, b, actual);
    check_bytes("vpxor xmm memory clears high", actual, expected, 32);

    memset(actual, 0xa5, sizeof(actual));
    avx_vextracti128(a, actual);
    memset(expected, 0, 32);
    memcpy(expected, a, 16);
    check_bytes("vextracti128 xmm imm0 clears high", actual, expected, 32);
    memset(expected, 0, 32);
    memcpy(expected, a + 16, 16);
    check_bytes("vextracti128 xmm imm1 clears high", actual + 32,
                expected, 32);
    check_bytes("vextracti128 memory imm0", actual + 64, a, 16);
    check_bytes("vextracti128 memory imm1", actual + 80, a + 16, 16);

    memset(expected, 0, 64);
    memcpy(expected, a, 16);
    memcpy(expected + 32, a + 32, 16);
    avx_vzeroupper(a, actual);
    check_bytes("vzeroupper ymm0 and ymm15", actual, expected, 64);

    memset(expected, 0, 32);
    memcpy(expected, a, 16);
    memcpy(expected, b, 4);
    avx_vmovss_three(a, b, actual);
    check_bytes("vmovss three operand clears high", actual, expected, 32);

    memset(expected, 0, 32);
    memcpy(expected, b, 4);
    avx_vmovss_load(a, b, actual);
    check_bytes("vmovss scalar load clears upper", actual, expected, 32);

    memset(actual, 0xa5, sizeof(actual));
    avx_vmovss_store(a, actual);
    check_bytes("vmovss scalar store", actual, a, 4);
    memset(expected, 0xa5, 12);
    check_bytes("vmovss scalar store width", actual + 4, expected, 12);

    avx_cross_tb(a, actual);
    check_bytes("YMM shadow across TB", actual, a, 32);
    avx_across_syscall(a, actual);
    check_bytes("YMM shadow across syscall helper", actual, a, 32);

    if (failures) {
        printf("RESULT FAIL count=%d\n", failures);
        return 1;
    }
    puts("RESULT PASS all six AVX instruction semantics");
    return 0;
}
