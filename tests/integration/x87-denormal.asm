; SPDX-License-Identifier: GPL-2.0-only
%ifidn __OUTPUT_FORMAT__, elf64
BITS 64
default rel
%else
BITS 32
%endif
%assign case_id 1

%macro check_status 1
    fnstsw ax
    and ax, 7             ; invalid, denormal operand, divide-by-zero
    mov ebx, case_id
    cmp ax, %1
    jne exit
%assign case_id case_id + 1
%endmacro
%macro binary 4
    fninit
    fld tword [%3]
    fld tword [%2]
    fnclex
%ifidni %1, fcom
    fcom st1
%elifidni %1, fucom
    fucom st1
%elifidni %1, fscale
    fscale
%elifidni %1, fprem
    fprem
%elifidni %1, fprem1
    fprem1
%elifidni %1, fpatan
    fpatan
%elifidni %1, fyl2x
    fyl2x
%elifidni %1, fyl2xp1
    fyl2xp1
%else
    %1 st0, st1
%endif
    check_status %4
%endmacro
%macro unary 3
    fninit
    fld tword [%2]
    fnclex
    %1
    check_status %3
%endmacro
%macro arithmetic_cases 1
    binary %1, tiny, one, 2
    binary %1, one, tiny, 2
    binary %1, negative_tiny, one, 2
    binary %1, pseudo, one, 2
    binary %1, one, pseudo, 2
    binary %1, normal, one, 0
    binary %1, zero, one, 0
    binary %1, tiny, qnan, 0
    binary %1, qnan, tiny, 0
    binary %1, tiny, snan, 1
    binary %1, snan, tiny, 1
    binary %1, tiny, unsupported, 1
%endmacro
%macro compare_cases 2
    binary %1, tiny, one, 2
    binary %1, one, tiny, 2
    binary %1, pseudo, one, 2
    binary %1, normal, one, 0
    binary %1, tiny, qnan, %2
    binary %1, qnan, tiny, %2
    binary %1, tiny, snan, 1
    binary %1, tiny, unsupported, 1
%endmacro
%macro memory_case 4
    fninit
    fld tword [%2]
    fnclex
    %1 %3
    check_status %4
%endmacro
section .text
global _start
_start:
    arithmetic_cases fadd
    arithmetic_cases fsub
    arithmetic_cases fmul
    arithmetic_cases fdiv
    binary fdiv, tiny, zero, 4
    binary fdiv, zero, tiny, 2
    compare_cases fcom, 1
    compare_cases fcomi, 1
    compare_cases fucom, 0
    compare_cases fucomi, 0
    binary fscale, tiny, zero, 2
    binary fprem, tiny, one, 2
    binary fprem1, tiny, one, 2
    binary fpatan, tiny, one, 2
    binary fyl2x, tiny, one, 2
    binary fyl2xp1, tiny, one, 2
    memory_case fadd, one, dword [single_tiny], 2
    memory_case fadd, qnan, dword [single_tiny], 0
    memory_case fadd, snan, dword [single_tiny], 1
    memory_case fadd, one, qword [double_tiny], 2
    memory_case fadd, qnan, qword [double_tiny], 0
    memory_case fadd, snan, qword [double_tiny], 1
    memory_case fsub, one, dword [single_tiny], 2
    memory_case fsub, qnan, dword [single_tiny], 0
    memory_case fsub, snan, dword [single_tiny], 1
    memory_case fsub, one, qword [double_tiny], 2
    memory_case fsub, qnan, qword [double_tiny], 0
    memory_case fsub, snan, qword [double_tiny], 1
    memory_case fmul, one, dword [single_tiny], 2
    memory_case fmul, qnan, dword [single_tiny], 0
    memory_case fmul, snan, dword [single_tiny], 1
    memory_case fmul, one, qword [double_tiny], 2
    memory_case fmul, qnan, qword [double_tiny], 0
    memory_case fmul, snan, qword [double_tiny], 1
    memory_case fdiv, one, dword [single_tiny], 2
    memory_case fdiv, qnan, dword [single_tiny], 0
    memory_case fdiv, snan, dword [single_tiny], 1
    memory_case fdiv, one, qword [double_tiny], 2
    memory_case fdiv, qnan, qword [double_tiny], 0
    memory_case fdiv, snan, qword [double_tiny], 1
    memory_case fcom, one, dword [single_tiny], 2
    memory_case fcom, qnan, dword [single_tiny], 1
    memory_case fcom, snan, dword [single_tiny], 1
    memory_case fcom, one, qword [double_tiny], 2
    memory_case fcom, qnan, qword [double_tiny], 1
    memory_case fcom, snan, qword [double_tiny], 1
    unary fsqrt, tiny, 2
    unary fsqrt, pseudo, 2
    unary fsqrt, negative_tiny, 1
    unary frndint, tiny, 2
    unary f2xm1, tiny, 2
    unary fsin, tiny, 2
    unary fcos, tiny, 2
    unary fsincos, tiny, 2
    unary fptan, tiny, 2
    unary fxtract, tiny, 2
    ; These instructions do not raise D for an extended input.
    unary fabs, tiny, 0
    unary fchs, tiny, 0
    unary fxam, tiny, 0
    fninit
    fld tword [tiny]
    check_status 0
    fst dword [result]
    check_status 0
    fist dword [result]
    check_status 0
    fninit
    fld dword [single_tiny]
    check_status 2
    fnclex
    check_status 0
    fninit
    fld qword [double_tiny]
    check_status 2
    fninit
    check_status 0
    ; D remains sticky across a subsequent normal operation.
    fld tword [tiny]
    fsqrt
    fstp tword [result]
    fld1
    fsqrt
    check_status 2
    fnstenv [saved_env]
    fnclex
    fldenv [saved_env]
    check_status 2
    xor ebx, ebx
exit:
%ifidn __OUTPUT_FORMAT__, elf64
    mov edi, ebx
    mov eax, 60
    syscall
%else
    mov eax, 1
    int 0x80
%endif
section .data align=16
tiny: dq 1
      dw 0
negative_tiny: dq 1
               dw 0x8000
pseudo: dq 0x8000000000000000
        dw 0
normal: dq 0x8000000000000000
        dw 1
one: dq 0x8000000000000000
     dw 0x3fff
zero: dq 0
      dw 0
qnan: dq 0xc000000000000001
      dw 0x7fff
snan: dq 0x8000000000000001
      dw 0x7fff
unsupported: dq 1
             dw 1
align 8
single_tiny: dd 1
align 8
double_tiny: dq 1
section .bss align=16
result: resb 16
saved_env: resb 32
section .note.GNU-stack noalloc noexec nowrite progbits
