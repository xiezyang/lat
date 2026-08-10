/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_main
    .type latx_avx_single_main, @function
latx_avx_single_main:
    mov eax, 0x3f
    mov ecx, 0x15
    pext eax, eax, ecx
    cmp eax, 7
    sete al
    movzx eax, al
    xor eax, 1
    ret

    .section .note.GNU-stack,"",@progbits
