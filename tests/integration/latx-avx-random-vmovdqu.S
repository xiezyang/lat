/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_random_vmovdqu
    .type latx_avx_random_vmovdqu, @function
latx_avx_random_vmovdqu:
    vmovdqu ymm15, ymmword ptr [rdi + 1]
    vmovdqu ymm0, ymm15
    vmovdqu ymmword ptr [rdx + 1], ymm0

    vmovdqu ymm14, ymmword ptr [rdi + r8 * 8 + 129]
    vmovdqu ymm1, ymm14
    vmovdqu ymmword ptr [rdx + 33], ymm1

    vmovdqu ymm13, ymmword ptr [rcx + 1]
    vmovdqu ymmword ptr [rcx + 33], ymm13
    vmovdqu ymm12, ymmword ptr [rcx + 33]
    vmovdqu ymmword ptr [rdx + 65], ymm12

    vmovdqu ymm11, ymmword ptr [rdi + 65]
    vmovdqu xmm11, xmmword ptr [rdi + 97]
    vmovdqu ymmword ptr [rdx + 97], ymm11

    vmovdqu ymm10, ymmword ptr [rdi + 129]
    vmovdqu xmm9, xmmword ptr [rdi + 145]
    vmovdqu xmm10, xmm9
    vmovdqu ymmword ptr [rdx + 129], ymm10

    vmovdqu xmm8, xmmword ptr [rdi + 161]
    vmovdqu xmmword ptr [rdx + 193], xmm8
    ret

    .section .note.GNU-stack,"",@progbits
