; SPDX-License-Identifier: GPL-2.0-only
bits 64
default rel
global _start

; Check the branch and all six arithmetic flags after either exit.
; MOVAPD deliberately destroys a compare input without changing EFLAGS.
%macro CHECK 7
    mov ecx, 64
%%again:
    movaps xmm0, [%3]
    movaps xmm1, [%4]
    %1 xmm0, xmm1
%if %7
    movapd xmm0, [equal_value]
    lea r8, [rcx + 7]
%endif
    %2 %%taken
    mov r9d, 0
    jmp %%flags
%%taken:
    mov r9d, 1
%%flags:
    pushfq
    pop rax
    and eax, 0x8d5
    cmp eax, %6
    jne fail
    cmp r9d, (%5) & 1
    jne fail
%if %7
    lea r10, [rcx + 7]
    cmp r8, r10
    jne fail
%endif
    dec ecx
    jnz %%again
%endmacro

%macro RELATION 5
    CHECK %1, ja,  %2, %3, ((%4 & 0x41) = 0), %4, %5
    CHECK %1, jae, %2, %3, ((%4 & 1) = 0), %4, %5
    CHECK %1, jb,  %2, %3, ((%4 & 1) != 0), %4, %5
    CHECK %1, jbe, %2, %3, ((%4 & 0x41) != 0), %4, %5
    CHECK %1, je,  %2, %3, ((%4 & 0x40) != 0), %4, %5
    CHECK %1, jne, %2, %3, ((%4 & 0x40) = 0), %4, %5
    CHECK %1, jl,  %2, %3, 0, %4, %5
    CHECK %1, jge, %2, %3, 1, %4, %5
    CHECK %1, jle, %2, %3, ((%4 & 0x40) != 0), %4, %5
    CHECK %1, jg,  %2, %3, ((%4 & 0x40) = 0), %4, %5
%endmacro

%macro SUITE 3
    RELATION %1, two_%2, one_%2, 0, %3
    RELATION %1, one_%2, two_%2, 1, %3
    RELATION %1, one_%2, one_%2, 0x40, %3
    RELATION %1, nan_%2, one_%2, 0x45, %3
%endmacro

section .text
_start:
    ldmxcsr [clean]
%assign separated 0
%rep 2
    SUITE comiss, s, separated
    SUITE ucomiss, s, separated
    SUITE comisd, d, separated
    SUITE ucomisd, d, separated
    SUITE vcomiss, s, separated
    SUITE vucomiss, s, separated
    SUITE vcomisd, d, separated
    SUITE vucomisd, d, separated
%assign separated separated + 1
%endrep
    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, 60
    syscall

section .rodata align=16
one_s: dd 0x3f800000, 0, 0, 0
two_s: dd 0x40000000, 0, 0, 0
nan_s: dd 0x7fc00000, 0, 0, 0
one_d: dq 0x3ff0000000000000, 0
two_d: dq 0x4000000000000000, 0
nan_d: dq 0x7ff8000000000000, 0
equal_value: dq 0, 0
clean: dd 0x1f80
section .note.GNU-stack noalloc noexec nowrite progbits
