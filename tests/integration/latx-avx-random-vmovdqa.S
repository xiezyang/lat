/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_random_vmovdqa
    .type latx_avx_random_vmovdqa, @function
latx_avx_random_vmovdqa:
    vmovdqa ymm15, ymmword ptr [rdi]
    vmovdqa ymm0, ymm15
    vmovdqa ymmword ptr [rdx], ymm0

    vmovdqa ymm14, ymmword ptr [rdi + r8 * 8 + 128]
    vmovdqa ymm1, ymm14
    vmovdqa ymmword ptr [rdx + 32], ymm1

    vmovdqa ymm13, ymmword ptr [rcx]
    vmovdqa ymmword ptr [rcx + 32], ymm13
    vmovdqa ymm12, ymmword ptr [rcx + 32]
    vmovdqa ymmword ptr [rdx + 64], ymm12

    vmovdqa ymm11, ymmword ptr [rdi + 64]
    vmovdqa xmm11, xmmword ptr [rdi + 96]
    vmovdqa ymmword ptr [rdx + 96], ymm11

    vmovdqa ymm10, ymmword ptr [rdi + 128]
    vmovdqa xmm9, xmmword ptr [rdi + 144]
    vmovdqa xmm10, xmm9
    vmovdqa ymmword ptr [rdx + 128], ymm10

    vmovdqa xmm8, xmmword ptr [rdi + 160]
    vmovdqa xmmword ptr [rdx + 192], xmm8
    ret

    .section .note.GNU-stack,"",@progbits
