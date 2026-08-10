/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl _start
    .type _start, @function
_start:
    mov rdi, qword ptr [rsp]
    lea rsi, [rsp + 8]
    and rsp, -16
    call latx_avx_single_main

    mov edi, eax
    mov eax, 60
    syscall

    .section .note.GNU-stack,"",@progbits
