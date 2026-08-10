/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .data
    .balign 4
vmovd_ymm_memory:
    .long 0x4b5a6978

    .text
    .globl _start
    .type _start, @function
_start:
    cmp qword ptr [rsp], 2
    jb .Lusage
    mov rdx, qword ptr [rsp + 16]
    cmp byte ptr [rdx], '1'
    je .Lxmm15

    mov eax, 0x44332211
    mov ecx, 2
.Lxmm0_loop:
    vmovd xmm0, eax
    dec ecx
    jne .Lxmm0_loop
    jmp .Lsuccess

.Lxmm15:
    mov ecx, 2
.Lxmm15_loop:
    vmovd xmm15, dword ptr [rip + vmovd_ymm_memory]
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
