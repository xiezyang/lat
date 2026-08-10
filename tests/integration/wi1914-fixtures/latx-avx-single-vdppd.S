/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .globl latx_avx_single_vdppd_run
    .type latx_avx_single_vdppd_run, @function
latx_avx_single_vdppd_run:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm2, xmm0, xmm1, 0
    vmovdqu ymmword ptr [rdi + 0], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm0, xmm0, xmm1, 255
    vmovdqu ymmword ptr [rdi + 32], ymm0
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm1, xmm0, xmm1, 85
    vmovdqu ymmword ptr [rdi + 64], ymm1
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm2, xmm0, xmmword ptr [rip + input_b], 170
    vmovdqu ymmword ptr [rdi + 96], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm15, xmm15, xmm1, 0
    vmovdqu ymmword ptr [rdi + 128], ymm15
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm15, xmm0, xmm15, 255
    vmovdqu ymmword ptr [rdi + 160], ymm15
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm2, xmm0, xmm1, 85
    vmovdqu ymmword ptr [rdi + 192], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm0, xmm0, xmm1, 170
    vmovdqu ymmword ptr [rdi + 224], ymm0
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm1, xmm0, xmm1, 0
    vmovdqu ymmword ptr [rdi + 256], ymm1
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm2, xmm0, xmmword ptr [rip + input_b], 255
    vmovdqu ymmword ptr [rdi + 288], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm15, xmm15, xmm1, 85
    vmovdqu ymmword ptr [rdi + 320], ymm15
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm15, xmm0, xmm15, 170
    vmovdqu ymmword ptr [rdi + 352], ymm15
    vzeroupper
    ret

    .globl latx_avx_single_vdppd_fault_xmm
    .type latx_avx_single_vdppd_fault_xmm, @function
latx_avx_single_vdppd_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vdppd xmm15, xmm0, xmmword ptr [rdi], 255
    ret

    .section .rodata
    .balign 32
input_a:
    .quad 0x0000000080000000, 0x8000000000000000
    .quad 0x7ff8000000000042, 0x7ff0000000000001
input_b:
    .quad 0x0000000000000000, 0x8000000000000000
    .quad 0x0000000000000001, 0x7ff0000000000000

    .section .note.GNU-stack,"",@progbits
