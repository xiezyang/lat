/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .globl latx_avx_single_vaesdeclast_run
    .type latx_avx_single_vaesdeclast_run, @function
latx_avx_single_vaesdeclast_run:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vaesdeclast xmm2, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 0], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vaesdeclast xmm0, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 32], ymm0
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vaesdeclast xmm1, xmm0, xmm1
    vmovdqu ymmword ptr [rdi + 64], ymm1
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vaesdeclast xmm2, xmm0, xmmword ptr [rip + input_b]
    vmovdqu ymmword ptr [rdi + 96], ymm2
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmovdqu xmm15, xmmword ptr [rip + input_a]
    vaesdeclast xmm15, xmm15, xmm1
    vmovdqu ymmword ptr [rdi + 128], ymm15
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmovdqu xmm15, xmmword ptr [rip + input_b]
    vaesdeclast xmm15, xmm0, xmm15
    vmovdqu ymmword ptr [rdi + 160], ymm15
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vaesdeclast ymm2, ymm0, ymm1
    vmovdqu ymmword ptr [rdi + 192], ymm2
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vaesdeclast ymm0, ymm0, ymm1
    vmovdqu ymmword ptr [rdi + 224], ymm0
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vaesdeclast ymm1, ymm0, ymm1
    vmovdqu ymmword ptr [rdi + 256], ymm1
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vaesdeclast ymm2, ymm0, ymmword ptr [rip + input_b]
    vmovdqu ymmword ptr [rdi + 288], ymm2
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmovdqu ymm15, ymmword ptr [rip + input_a]
    vaesdeclast ymm15, ymm15, ymm1
    vmovdqu ymmword ptr [rdi + 320], ymm15
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmovdqu ymm15, ymmword ptr [rip + input_b]
    vaesdeclast ymm15, ymm0, ymm15
    vmovdqu ymmword ptr [rdi + 352], ymm15
    vzeroupper
    ret

    .globl latx_avx_single_vaesdeclast_fault_xmm
    .type latx_avx_single_vaesdeclast_fault_xmm, @function
latx_avx_single_vaesdeclast_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vaesdeclast xmm15, xmm0, xmmword ptr [rdi]
    ret
    .globl latx_avx_single_vaesdeclast_fault_ymm
    .type latx_avx_single_vaesdeclast_fault_ymm, @function
latx_avx_single_vaesdeclast_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vaesdeclast ymm15, ymm0, ymmword ptr [rdi]
    ret

    .section .rodata
    .balign 32
input_a:
    .quad 0x0001020304050607, 0x08090a0b0c0d0e0f
    .quad 0x1011121314151617, 0x18191a1b1c1d1e1f
input_b:
    .quad 0xffeeddccbbaa9988, 0x7766554433221100
    .quad 0x0123456789abcdef, 0xfedcba9876543210

    .section .note.GNU-stack,"",@progbits
