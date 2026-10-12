; SPDX-License-Identifier: GPL-2.0-only
bits 64
default rel
global _start

section .text
_start:
%assign reg 0
%rep 16
    vmovdqu ymm %+ reg, [ones]
%assign reg reg + 1
%endrep
    vzeroupper
    vzeroall
%assign reg 0
%rep 16
    vmovdqu [actual + reg * 32], ymm %+ reg
%assign reg reg + 1
%endrep
    lea rsi, [actual]
    mov ecx, 64
check:
    cmp qword [rsi], 0
    jne fail
    add rsi, 8
    loop check
    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, 60
    syscall

section .rodata align=32
ones: times 4 dq -1

section .bss align=32
actual: resb 512

section .note.GNU-stack noalloc noexec nowrite progbits
