/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_main
    .type latx_avx_single_main, @function
latx_avx_single_main:
    movdqu xmm0, XMMWORD PTR [rip + vex_input]
    /* VEX.128 VPSRLQ xmm0, xmm0, 1. */
    .byte 0xc5, 0xf9, 0x73, 0xd0, 0x01
    xor eax, eax
    ret

    .section .rodata
    .balign 16
vex_input:
    .quad 4, 8

    .section .note.GNU-stack,"",@progbits
