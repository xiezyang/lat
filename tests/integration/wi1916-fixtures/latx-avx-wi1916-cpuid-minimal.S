/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .globl latx_avx_wi1916_cpuid_minimal_run
    .type latx_avx_wi1916_cpuid_minimal_run, @function
latx_avx_wi1916_cpuid_minimal_run:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vaesdec xmm0, xmm0, xmm1
    vmovdqu xmmword ptr [rdi], xmm0
    ret

    .section .rodata
    .balign 16
input_a:
    .quad 0x0001020304050607, 0x08090a0b0c0d0e0f
input_b:
    .quad 0xffeeddccbbaa9988, 0x7766554433221100

    .section .note.GNU-stack,"",@progbits
