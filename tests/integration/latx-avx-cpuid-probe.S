/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .globl execute_vex_vpsrlq
    .type execute_vex_vpsrlq, @function
execute_vex_vpsrlq:
    movdqu xmm0, XMMWORD PTR [rip + execute_vex_vpsrlq_input]
    /* VEX.128 VPSRLQ xmm0, xmm0, 1. */
    .byte 0xc5, 0xf9, 0x73, 0xd0, 0x01
    movq rax, xmm0
    ret

    .section .rodata
    .balign 16
execute_vex_vpsrlq_input:
    .quad 4, 8

    .section .note.GNU-stack,"",@progbits
