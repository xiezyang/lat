/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_random_vmovss
    .type latx_avx_random_vmovss, @function
latx_avx_random_vmovss:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu ymm1, ymmword ptr [rdi + 32]
    vmovdqu ymm2, ymmword ptr [rsi]
    vmovss xmm0, xmm1, xmm2
    vmovdqu ymmword ptr [rdx], ymm0

    vmovdqu ymm3, ymmword ptr [rdi + 64]
    vmovdqu ymm4, ymmword ptr [rsi + 32]
    vmovss xmm3, xmm3, xmm4
    vmovdqu ymmword ptr [rdx + 32], ymm3

    vmovdqu ymm5, ymmword ptr [rdi + 96]
    vmovdqu ymm6, ymmword ptr [rsi + 64]
    vmovss xmm6, xmm5, xmm6
    vmovdqu ymmword ptr [rdx + 64], ymm6

    vmovdqu ymm7, ymmword ptr [rdi + 128]
    vmovss xmm7, xmm7, xmm7
    vmovdqu ymmword ptr [rdx + 96], ymm7

    vmovdqu ymm8, ymmword ptr [rdi + 160]
    vmovss xmm8, dword ptr [rsi + 1]
    vmovdqu ymmword ptr [rdx + 128], ymm8

    vmovdqu ymm9, ymmword ptr [rdi + 192]
    vmovss xmm9, dword ptr [rcx + 30]
    vmovdqu ymmword ptr [rdx + 160], ymm9

    vmovdqu ymm12, ymmword ptr [rdi + 288]
    vmovss xmm12, dword ptr [rsi + r8 * 4 + 129]
    vmovdqu ymmword ptr [rdx + 192], ymm12

    vmovdqu ymm10, ymmword ptr [rdi + 224]
    vmovss dword ptr [rdx + 256], xmm10

    vmovdqu ymm11, ymmword ptr [rdi + 256]
    vmovss dword ptr [rcx + 30], xmm11
    mov eax, dword ptr [rcx + 30]
    mov dword ptr [rdx + 288], eax
    ret

    .section .note.GNU-stack,"",@progbits
