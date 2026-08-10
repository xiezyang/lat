/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_vpxor_run
    .type latx_avx_single_vpxor_run, @function
latx_avx_single_vpxor_run:
    movdqu xmm0, xmmword ptr [rip + vpxor_input_a]
    movdqu xmm1, xmmword ptr [rip + vpxor_input_b]
    vpxor xmm2, xmm0, xmm1
    movdqu xmmword ptr [rdi], xmm2

    movdqu xmm3, xmmword ptr [rip + vpxor_input_a + 16]
    movdqu xmm4, xmmword ptr [rip + vpxor_input_b + 16]
    vpxor xmm5, xmm3, xmm4
    movdqu xmmword ptr [rdi + 16], xmm5

    /* Exercise the YMM path with a memory operand and a zeroed high half. */
    vpxor ymm6, ymm6, ymm6
    vpxor ymm6, ymm6, ymmword ptr [rip + vpxor_input_a]

    ret

    .section .data
    .balign 32
vpxor_input_a:
    .quad 0x0011223344556677, 0x8899aabbccddeeff
    .quad 0x1021324354657687, 0x98a9bacbdcedfe0f
vpxor_input_b:
    .quad 0xffeeddccbbaa9988, 0x7766554433221100
    .quad 0xf0decba987654321, 0x0ffeeddccbbaa9988

    .section .note.GNU-stack,"",@progbits
