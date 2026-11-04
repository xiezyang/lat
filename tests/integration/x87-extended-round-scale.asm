; SPDX-License-Identifier: GPL-2.0-only
BITS 32

%macro check80 2
    cmp dword [result], %1
    jne fail
    cmp dword [result + 4], 0x80000000
    jne fail
    cmp word [result + 8], %2
    jne fail
%endmacro

section .text
global _start
_start:
    fninit
    fld tword [large_integer]
    frndint
    fstp tword [result]
    check80 1, 0x403e

    fninit
    fild dword [negative_scale]
    fld1
    fscale
    fstp tword [result]
    check80 0, 0x3bcd
    fstp tword [result]

    fninit
    fild dword [positive_scale]
    fld1
    fscale
    fstp tword [result]
    check80 0, 0x47cf
    fstp tword [result]
    xor ebx, ebx
    jmp exit
fail:
    mov ebx, 1
exit:
    mov eax, 1
    int 0x80

section .data
large_integer: dq 0x8000000000000001
               dw 0x403e
negative_scale: dd -1074
positive_scale: dd 2000
section .bss
result: resb 10
section .note.GNU-stack noalloc noexec nowrite progbits
