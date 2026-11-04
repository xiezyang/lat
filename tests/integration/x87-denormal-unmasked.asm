; SPDX-License-Identifier: GPL-2.0-only
%ifndef TEST_CASE
%define TEST_CASE 1
%endif
%ifidn __OUTPUT_FORMAT__, elf64
BITS 64
default rel
%else
BITS 32
%endif
section .text
global _start
_start:
    fninit
%if TEST_CASE == 1 || TEST_CASE == 6
%define expected_sw 0xb882
%else
    fld1
%define expected_sw 0xb082
%endif
    fld tword [tiny]
    fldcw [control]
    fnclex
%if TEST_CASE == 1
    fsqrt
%elif TEST_CASE == 2
    faddp st1, st0
%elif TEST_CASE == 3
    fcomip st0, st1
%elif TEST_CASE == 4
    fmul st0, st1
%elif TEST_CASE == 5
    fdiv st0, st1
%elif TEST_CASE == 6
    fninit
    fldcw [control]
    fld dword [single_tiny]
%endif
    ; Non-waiting state inspection must precede FWAIT.
    fnstsw [status]
    mov ax, [status]
    and ax, 0xb8bf
    cmp ax, expected_sw
    jne fail
    fnstenv [saved]
    fnclex
    fstp tword [result]
%if TEST_CASE == 6
    cmp dword [result], 0
    jne fail
    cmp dword [result + 4], 0x80000000
    jne fail
    cmp word [result + 8], 0x3f6a
    jne fail
%else
    cmp dword [result], 1
    jne fail
    cmp dword [result + 4], 0
    jne fail
    cmp word [result + 8], 0
    jne fail
%endif
    fldenv [saved]
    fnstsw [status]
%ifidn __OUTPUT_FORMAT__, elf64
    mov eax, 1
    mov edi, 1
    lea rsi, [status]
    mov edx, 2
    syscall
%else
    mov eax, 4
    mov ebx, 1
    mov ecx, status
    mov edx, 2
    int 0x80
%endif
    fwait                 ; Must report SIGFPE after the non-waiting checks.
fail:
%ifidn __OUTPUT_FORMAT__, elf64
    mov eax, 60
    mov edi, 1
    syscall
%else
    mov eax, 1
    mov ebx, 1
    int 0x80
%endif
section .data
control: dw 0x037d
tiny: dq 1
      dw 0
align 4
single_tiny: dd 1
section .bss align=16
saved: resb 108
result: resb 16
status: resw 1
section .note.GNU-stack noalloc noexec nowrite progbits
