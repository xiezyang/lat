; SPDX-License-Identifier: GPL-2.0-only
bits 64
default rel
global _start

%macro CHECK 2
    vmovdqu [actual], %1
    cmp dword [actual], %2
    jne fail
    cmp dword [actual + 4], 0x11223344
    jne fail
    cmp dword [actual + 8], 0x55667788
    jne fail
    cmp dword [actual + 12], 0x12345678
    jne fail
    cmp qword [actual + 16], 0
    jne fail
    cmp qword [actual + 24], 0
    jne fail
%endmacro

%macro ARITH 2
    vmovdqu ymm0, [a]
    vmovdqu ymm1, [b]
    vmovdqu ymm2, [c]
    %1 xmm2, xmm0, xmm1
    CHECK ymm2, %2
    %1 xmm0, xmm0, xmm1
    CHECK ymm0, %2
    vmovdqu ymm0, [a]
    %1 xmm0, xmm0, [b]
    CHECK ymm0, %2
%endmacro

%macro FMA 2
    vmovdqu ymm0, [a]
    vmovdqu ymm1, [b]
    vmovdqu ymm2, [c]
    %1 xmm0, xmm1, xmm2
    CHECK ymm0, %2
%endmacro

section .text
_start:
    ldmxcsr [clean]
    ARITH vaddss, 0x40a00000     ; 2 + 3 = 5
    ARITH vsubss, 0xbf800000     ; 2 - 3 = -1
    ARITH vmulss, 0x40c00000     ; 2 * 3 = 6
    ARITH vdivss, 0x3f2aaaab     ; 2 / 3
    FMA vfmadd132ss, 0x41300000 ; 2 * 4 + 3 = 11
    FMA vfmadd213ss, 0x41200000 ; 3 * 2 + 4 = 10
    FMA vfmadd231ss, 0x41600000 ; 3 * 4 + 2 = 14
    FMA vfmsub132ss, 0x40a00000 ; 2 * 4 - 3 = 5
    FMA vfmsub213ss, 0x40000000 ; 3 * 2 - 4 = 2
    FMA vfmsub231ss, 0x41200000 ; 3 * 4 - 2 = 10

    vmovdqu ymm0, [a]
    vmovss xmm0, [b]
    vmovdqu [actual], ymm0
    cmp dword [actual], 0x40400000
    jne fail
    cmp dword [actual + 4], 0
    jne fail
    cmp qword [actual + 8], 0
    jne fail
    cmp qword [actual + 16], 0
    jne fail
    cmp qword [actual + 24], 0
    jne fail

    ; Only scalar values are observed in this hot loop.
    mov ecx, 300000
again:
    vmovss xmm0, [a]
    vmovss xmm1, [b]
    vaddss xmm2, xmm0, xmm1
    vmulss xmm2, xmm2, xmm0
    vfmsub231ss xmm2, xmm0, xmm1
    vmovss [scalar], xmm2     ; 2*3 - ((2+3)*2) = -4
    dec ecx
    jne again
    cmp dword [scalar], 0xc0800000
    jne fail
    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, 60
    syscall

section .rodata align=32
a: dd 0x40000000, 0x11223344, 0x55667788, 0x12345678
   dq -1, -1
b: dd 0x40400000, 0, 0, 0
   dq -1, -1
c: dd 0x40800000, 0, 0, 0
   dq -1, -1
clean: dd 0x1f80

section .bss align=32
actual: resb 32
scalar: resd 1

section .note.GNU-stack noalloc noexec nowrite progbits
