/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix

    .section .text
    .globl _start
    .type _start, @function
_start:
    mov ecx, 2
.Lrepeat:
    vzeroupper
    dec ecx
    jne .Lrepeat

    mov eax, 60
    xor edi, edi
    syscall

    .section .note.GNU-stack,"",@progbits
