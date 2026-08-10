/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .globl latx_avx_single_vmaxss_run
    .type latx_avx_single_vmaxss_run, @function
latx_avx_single_vmaxss_run:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm2, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 0], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm0, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 32], ymm0
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm1, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 64], ymm1
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm2, xmm0, dword ptr [rip + input_b]
    vmovdqu ymmword ptr [rdi + 96], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm15, xmm15, xmm1
    vmovdqu ymmword ptr [rdi + 128], ymm15
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm2, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 160], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm0, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 192], ymm0
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm1, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 224], ymm1
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm2, xmm0, dword ptr [rip + input_b]
    vmovdqu ymmword ptr [rdi + 256], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm15, xmm15, xmm1
    vmovdqu ymmword ptr [rdi + 288], ymm15
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm2, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 320], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm0, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 352], ymm0
    vzeroupper
    ret

    .globl latx_avx_single_vmaxss_fault_xmm
    .type latx_avx_single_vmaxss_fault_xmm, @function
latx_avx_single_vmaxss_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmaxss xmm15, xmm0, dword ptr [rdi]
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
