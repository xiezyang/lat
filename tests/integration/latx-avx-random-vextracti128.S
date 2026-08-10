/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_random_vextracti128
    .type latx_avx_random_vextracti128, @function
latx_avx_random_vextracti128:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu ymm15, ymmword ptr [rdi + 32]

    vextracti128 xmm1, ymm0, 0
    vmovdqu ymmword ptr [rdx], ymm1

    vextracti128 xmm15, ymm0, 1
    vmovdqu ymmword ptr [rdx + 32], ymm15

    vextracti128 xmm0, ymm0, 255
    vmovdqu ymmword ptr [rdx + 64], ymm0

    vmovdqu ymm15, ymmword ptr [rdi + 32]
    vextracti128 xmmword ptr [rdx + 96], ymm15, 0
    vextracti128 xmmword ptr [rdx + 112], ymm15, 1
    vextracti128 xmmword ptr [rcx + 17], ymm15, 2
    vmovdqu xmm14, xmmword ptr [rcx + 17]
    vmovdqu xmmword ptr [rdx + 128], xmm14

    vextracti128 xmmword ptr [rdx + r8 * 8 + 256], ymm15, 255
    ret

    .section .note.GNU-stack,"",@progbits
