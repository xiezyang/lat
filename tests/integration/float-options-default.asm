; SPDX-License-Identifier: GPL-2.0-only
%ifdef GUEST32
BITS 32
%else
BITS 64
DEFAULT REL
%endif

%macro scalar 4
    movdqu xmm0, [%2]
    movdqu xmm1, [%3]
    %1 xmm0, xmm1
    pcmpeqd xmm0, [%4]
    pmovmskb eax, xmm0
    cmp eax, 0xffff
    jne fail
%endmacro

section .text
global _start
_start:
    fninit
    fld1
    fld1
    faddp st1, st0
    fistp dword [result]
    cmp dword [result], 2
    jne fail
    fld1
    fchs
    fabs
    fld1
    fucomip st0, st1
    jne fail
    fstp st0
    fld1
    fldz
    stc
    fcmovb st0, st1
    fistp dword [result]
    cmp dword [result], 1
    jne fail
    fstp st0
    scalar addss, single_one, single_two, single_three
    scalar subss, single_three, single_two, single_one
    scalar mulss, single_three, single_two, single_six
    scalar addsd, double_one, double_two, double_three
    scalar subsd, double_three, double_two, double_one
    scalar mulsd, double_three, double_two, double_six
    xor ebx, ebx
    jmp exit
fail:
    mov ebx, 1
exit:
%ifdef GUEST32
    mov eax, 1
    int 0x80
%else
    mov edi, ebx
    mov eax, 60
    syscall
%endif

section .data
align 16
single_one: dd 1.0, 0x12345678, 0x76543210, 0x55aa55aa
single_two: dd 2.0, 0, 0, 0
single_three: dd 3.0, 0x12345678, 0x76543210, 0x55aa55aa
single_six: dd 6.0, 0x12345678, 0x76543210, 0x55aa55aa
double_one: dq 1.0, 0x1234567876543210
double_two: dq 2.0, 0
double_three: dq 3.0, 0x1234567876543210
double_six: dq 6.0, 0x1234567876543210
result: dd 0

section .note.GNU-stack noalloc noexec nowrite progbits
