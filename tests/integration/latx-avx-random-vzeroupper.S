/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_random_vzeroupper
    .type latx_avx_random_vzeroupper, @function
latx_avx_random_vzeroupper:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu ymm1, ymmword ptr [rdi + 32]
    vmovdqu ymm2, ymmword ptr [rdi + 64]
    vmovdqu ymm3, ymmword ptr [rdi + 96]
    vmovdqu ymm4, ymmword ptr [rdi + 128]
    vmovdqu ymm5, ymmword ptr [rdi + 160]
    vmovdqu ymm6, ymmword ptr [rdi + 192]
    vmovdqu ymm7, ymmword ptr [rdi + 224]
    vmovdqu ymm8, ymmword ptr [rdi + 256]
    vmovdqu ymm9, ymmword ptr [rdi + 288]
    vmovdqu ymm10, ymmword ptr [rdi + 320]
    vmovdqu ymm11, ymmword ptr [rdi + 352]
    vmovdqu ymm12, ymmword ptr [rdi + 384]
    vmovdqu ymm13, ymmword ptr [rdi + 416]
    vmovdqu ymm14, ymmword ptr [rdi + 448]
    vmovdqu ymm15, ymmword ptr [rdi + 480]

    vzeroupper

    vmovdqu ymmword ptr [rdx], ymm0
    vmovdqu ymmword ptr [rdx + 32], ymm1
    vmovdqu ymmword ptr [rdx + 64], ymm2
    vmovdqu ymmword ptr [rdx + 96], ymm3
    vmovdqu ymmword ptr [rdx + 128], ymm4
    vmovdqu ymmword ptr [rdx + 160], ymm5
    vmovdqu ymmword ptr [rdx + 192], ymm6
    vmovdqu ymmword ptr [rdx + 224], ymm7
    vmovdqu ymmword ptr [rdx + 256], ymm8
    vmovdqu ymmword ptr [rdx + 288], ymm9
    vmovdqu ymmword ptr [rdx + 320], ymm10
    vmovdqu ymmword ptr [rdx + 352], ymm11
    vmovdqu ymmword ptr [rdx + 384], ymm12
    vmovdqu ymmword ptr [rdx + 416], ymm13
    vmovdqu ymmword ptr [rdx + 448], ymm14
    vmovdqu ymmword ptr [rdx + 480], ymm15
    ret

    .section .note.GNU-stack,"",@progbits
