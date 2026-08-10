/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl _start
    .type _start, @function
_start:
    cmp qword ptr [rsp], 2
    jb .Lusage
    mov rdx, qword ptr [rsp + 16]
    cmp byte ptr [rdx], '1'
    je .Lxmm15

    movabs rax, 0x8877665544332211
    mov ecx, 2
.Lxmm0_loop:
    vmovq xmm0, rax
    dec ecx
    jne .Lxmm0_loop
    jmp .Lsuccess

.Lxmm15:
    movabs r15, 0x0f1e2d3c4b5a6978
    mov ecx, 2
.Lxmm15_loop:
    vmovq xmm15, r15
    dec ecx
    jne .Lxmm15_loop

.Lsuccess:
    xor edi, edi
    jmp .Lexit
.Lusage:
    mov edi, 2
.Lexit:
    mov eax, 60
    syscall

    .section .note.GNU-stack,"",@progbits
