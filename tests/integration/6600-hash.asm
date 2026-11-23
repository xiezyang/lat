; SPDX-License-Identifier: GPL-2.0-only
; Alternate targets that collide under HASH mode 1. No floating-point or AVX.
global _start, target_a, target_b
%if __BITS__ = 64
default rel
%define PTR rax
%define CONT rdx
%else
%define PTR eax
%define CONT edx
%endif
section .text
_start:
    xor esi, esi
    mov edi, 10000
.loop:
    lea CONT, [.after_a]
    lea PTR, [target_a]
    jmp PTR
.after_a:
    lea CONT, [.after_b]
    lea PTR, [target_b]
    jmp PTR
.after_b:
    dec edi
    jnz .loop
    cmp esi, 30000
    jne .fail
    xor ebx, ebx
    jmp .exit
.fail:
    mov ebx, 1
.exit:
%if __BITS__ = 64
    mov edi, ebx
    mov eax, 60
    syscall
%else
    mov eax, 1
    int 0x80
%endif
align 131072
; Also use an index above 4095 to catch an accidental 12-bit cache limit.
times 4096 db 0x90
target_a:
    inc esi
    jmp CONT
; Toggle address bits 0 and 16 together: the 16-bit XOR index stays equal.
times 65537 - ($ - target_a) db 0x90
target_b:
    add esi, 2
    jmp CONT
section .note.GNU-stack noalloc noexec nowrite progbits
