; SPDX-License-Identifier: GPL-2.0-only
; Built as elf32 or elf64. Compare all 256 guest-visible bits after each case.
%if __BITS__ = 64
default rel
%endif
global _start

%macro CHECK 2
    vmovdqu [actual], ymm0
%assign offset 0
%rep 8
    mov eax, [actual + offset]
    cmp eax, [%1 + offset]
    jne fail%2
%assign offset offset + 4
%endrep
%endmacro

section .text
_start:
    vmovdqu ymm0, [input1]
    vpxor xmm0, xmm0, xmm0
    CHECK zero, 1

    vmovdqu ymm0, [input1]
    vmovdqu xmm0, [input2]
    CHECK input2_xmm, 2

    vmovdqu ymm0, [input1]
    vmovdqu ymm1, [input1]
    vmovsldup xmm0, xmm1
    CHECK duplicate, 3

    vmovdqu ymm0, [input1]
    vmovdqu ymm1, [input1]
    vmovdqu ymm2, [input2]
    vmovss xmm0, xmm1, xmm2
    CHECK scalar, 4

    vmovdqu ymm0, [input1]
    pxor xmm0, xmm0
    CHECK legacy, 5

    vmovdqu ymm0, [input1]
    vzeroupper
    CHECK input1_xmm, 6

    vmovdqu ymm0, [input1]
    vmovdqu ymm1, [input1]
    vmovsldup xmm0, xmm1
    pxor xmm0, xmm0
    CHECK zero, 7

    vmovdqu ymm0, [input1]
    mov eax, 5
    movd xmm0, eax
    CHECK legacy_movd, 8

    xor ebx, ebx
    jmp exit
%assign id 1
%rep 8
fail%+id:
    mov ebx, id
    jmp exit
%assign id id + 1
%endrep
exit:
%if __BITS__ = 64
    mov edi, ebx
    mov eax, 60
    syscall
%else
    mov eax, 1
    int 0x80
%endif

section .rodata align=32
input1: dd 1, 2, 3, 4, 0x12345678, 0x76543210, 0x55aa55aa, 0xa55aa55a
input2: dd 5, 6, 7, 8, 0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00
zero: times 8 dd 0
input1_xmm: dd 1, 2, 3, 4, 0, 0, 0, 0
input2_xmm: dd 5, 6, 7, 8, 0, 0, 0, 0
duplicate: dd 1, 1, 3, 3, 0, 0, 0, 0
scalar: dd 5, 2, 3, 4, 0, 0, 0, 0
legacy: dd 0, 0, 0, 0, 0x12345678, 0x76543210, 0x55aa55aa, 0xa55aa55a
legacy_movd: dd 5, 0, 0, 0, 0x12345678, 0x76543210, 0x55aa55aa, 0xa55aa55a

section .bss align=32
actual: resb 32
section .note.GNU-stack noalloc noexec nowrite progbits
