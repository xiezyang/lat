; SPDX-License-Identifier: GPL-2.0-only
BITS 32

section .text
global _start
_start:
    xor esi, esi
.mode:
    xor ebp, ebp
.value:
    fninit
    mov eax, esi
    and eax, 3
    fldcw [controls + eax * 2]
    imul eax, ebp, 10
    lea edx, [values + eax]
    fld tword [edx]
    fist word [result]
    mov ax, [expected + esi * 2]
    cmp [result], ax
    jne fail
    fnstsw ax
    and ax, 0x3800
    cmp ax, 0x3800
    jne fail
    fstp tword [saved]
    mov eax, [edx]
    cmp [saved], eax
    jne fail
    mov eax, [edx + 4]
    cmp [saved + 4], eax
    jne fail
    mov ax, [edx + 8]
    cmp [saved + 8], ax
    jne fail

    fninit
    mov eax, esi
    and eax, 3
    fldcw [controls + eax * 2]
    fld tword [edx]
    fistp word [result]
    mov ax, [expected + esi * 2]
    cmp [result], ax
    jne fail
    fnstsw ax
    and ax, 0x3800
    jne fail
    inc ebp
    add esi, 4
    cmp ebp, value_count
    jb .value
    sub esi, value_count * 4
    inc esi
    cmp esi, 4
    jb .mode

%ifdef CHECK_NATIVE_FLAGS
    fninit
    fld tword [infinity]
    fistp dword [saved]
    cmp dword [saved], 0x80000000
    jne fail
    fnstsw ax
    test al, 1
    jz fail
    fninit
    fld tword [fraction]
    fistp word [result]
    cmp word [result], 2
    jne fail
    fnstsw ax
    test al, 0x20
    jz fail
    fninit
    fld tword [fraction]
    fistp dword [saved]
    cmp dword [saved], 2
    jne fail
    fnstsw ax
    test al, 0x20
    jz fail
%endif
    xor ebx, ebx
    jmp exit
fail:
    mov ebx, 1
exit:
    mov eax, 1
    int 0x80

section .data
; Each input has results for nearest, down, up, and toward zero.
controls: dw 0x037f, 0x077f, 0x0b7f, 0x0f7f
expected:
    dw 0x8000, 0x8000, 0x8000, 0x8000 ; 32769
    dw 0x8000, 0x8000, 0x8000, 0x8000 ; 65536
    dw 0x8000, 0x8000, 0x8000, 0x8000 ; -32769
    dw 0x8000, 0x8000, 0x8000, 0x8000 ; -65536
    dw 0x8000, 32767, 0x8000, 32767
    dw 32767, 32767, 0x8000, 32767
    dw -32768, 0x8000, -32768, -32768
    dw 0x8000, 0x8000, -32768, -32768
    dw 0x8000, 0x8000, 0x8000, 0x8000 ; 2147483648
    dw 0x8000, 0x8000, 0x8000, 0x8000 ; -2147483649
    dw 0, 0, 0, 0
    dw 0, 0, 0, 0
    dw 2, 1, 2, 1
    dw -2, -2, -1, -1
    dw 32767, 32767, 32767, 32767
    dw -32768, -32768, -32768, -32768
    dw 0x8000, 0x8000, 0x8000, 0x8000 ; infinity
    dw 0x8000, 0x8000, 0x8000, 0x8000 ; qNaN
values:
    dt 32769.0, 65536.0, -32769.0, -65536.0
    dt 32767.5, 32767.4, -32768.5, -32768.6
    dt 2147483648.0, -2147483649.0
    dt 0.0, -0.0, 1.5, -1.5, 32767.0, -32768.0
infinity:
    dq 0x8000000000000000
    dw 0x7fff
    dq 0xc000000000000001
    dw 0x7fff
value_count equ ($ - values) / 10
fraction: dt 1.5
section .bss
result: resw 1
saved: resb 10
section .note.GNU-stack noalloc noexec nowrite progbits
