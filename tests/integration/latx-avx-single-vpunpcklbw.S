/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .globl latx_avx_single_vpunpcklbw_run
    .type latx_avx_single_vpunpcklbw_run, @function
latx_avx_single_vpunpcklbw_run:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpunpcklbw xmm2, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 0], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpunpcklbw xmm0, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 32], ymm0
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpunpcklbw xmm1, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 64], ymm1
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpunpcklbw xmm0, xmm0, xmm0
    vmovdqu ymmword ptr [rdi + 96], ymm0
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpunpcklbw ymm2, ymm0, ymm1
    vmovdqu ymmword ptr [rdi + 128], ymm2
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpunpcklbw ymm0, ymm0, ymm1
    vmovdqu ymmword ptr [rdi + 160], ymm0
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpunpcklbw ymm1, ymm0, ymm1
    vmovdqu ymmword ptr [rdi + 192], ymm1
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpunpcklbw ymm0, ymm0, ymm0
    vmovdqu ymmword ptr [rdi + 224], ymm0
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpunpcklbw xmm2, xmm0, xmmword ptr [rip + input_b]
    vmovdqu ymmword ptr [rdi + 256], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpunpcklbw xmm0, xmm0, xmmword ptr [rip + input_b]
    vmovdqu ymmword ptr [rdi + 288], ymm0
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpunpcklbw ymm2, ymm0, ymmword ptr [rip + input_b]
    vmovdqu ymmword ptr [rdi + 320], ymm2
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpunpcklbw ymm0, ymm0, ymmword ptr [rip + input_b]
    vmovdqu ymmword ptr [rdi + 352], ymm0
    vzeroupper
    ret

    .section .rodata
    .balign 32
input_a:
    .quad 0x80017fff00010000, 0x7fff80010000ffff
    .quad 0x0123456789abcdef, 0xfedcba9876543210
input_b:
    .quad 0xffff00007fff8000, 0x00018000ffff7fff
    .quad 0x7ff8000000000042, 0x7ff0000000000001

    .section .note.GNU-stack,"",@progbits
