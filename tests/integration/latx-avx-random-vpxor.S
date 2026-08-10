/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_random_vpxor
    .type latx_avx_random_vpxor, @function
latx_avx_random_vpxor:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu ymm1, ymmword ptr [rsi]
    vpxor ymm2, ymm0, ymm1
    vmovdqu ymmword ptr [rdx], ymm2

    vmovdqu ymm3, ymmword ptr [rdi + 32]
    vmovdqu ymm4, ymmword ptr [rsi + 32]
    vpxor ymm3, ymm3, ymm4
    vmovdqu ymmword ptr [rdx + 32], ymm3

    vmovdqu ymm5, ymmword ptr [rdi + 64]
    vmovdqu ymm6, ymmword ptr [rsi + 64]
    vpxor ymm6, ymm5, ymm6
    vmovdqu ymmword ptr [rdx + 64], ymm6

    vmovdqu ymm7, ymmword ptr [rdi + 96]
    vpxor ymm7, ymm7, ymmword ptr [rcx + 1]
    vmovdqu ymmword ptr [rdx + 96], ymm7

    vmovdqu ymm15, ymmword ptr [rdi + 256]
    vpxor ymm15, ymm15, ymmword ptr [rsi + r8 * 8 + 129]
    vmovdqu ymmword ptr [rdx + 256], ymm15

    vmovdqu ymm8, ymmword ptr [rdi + 128]
    vmovdqu ymm9, ymmword ptr [rsi + 129]
    vpxor xmm10, xmm8, xmm9
    vmovdqu ymmword ptr [rdx + 128], ymm10

    vmovdqu ymm11, ymmword ptr [rdi + 160]
    vmovdqu ymm12, ymmword ptr [rsi + 161]
    vpxor xmm11, xmm11, xmm12
    vmovdqu ymmword ptr [rdx + 160], ymm11

    vmovdqu ymm13, ymmword ptr [rdi + 192]
    vmovdqu ymm14, ymmword ptr [rsi + 193]
    vpxor xmm14, xmm13, xmm14
    vmovdqu ymmword ptr [rdx + 192], ymm14

    vmovdqu ymm15, ymmword ptr [rdi + 224]
    vpxor xmm15, xmm15, xmmword ptr [rsi + 225]
    vmovdqu ymmword ptr [rdx + 224], ymm15

    vmovdqu ymm0, ymmword ptr [rdi + 288]
    vpxor xmm0, xmm0, xmmword ptr [rsi + r8 * 8 + 257]
    vmovdqu ymmword ptr [rdx + 288], ymm0
    ret

    .section .note.GNU-stack,"",@progbits
