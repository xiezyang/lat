/* SPDX-License-Identifier: GPL-2.0-only */
.intel_syntax noprefix
.text
.global _start
_start:
    mov eax, 13
    mov edi, 11
    lea rsi, [rip + action]
    xor edx, edx
    mov r10d, 8
    syscall
    test rax, rax
    js fail
    vmovdqu ymm5, [rip + ones]
    vmovdqu ymm8, [rip + ones]
    vmovdqu ymm6, [rip + ones]
    vpcmpeqd xmm5, xmm5, xmm5
    vpcmpeqd xmm8, xmm8, xmm8
    vpxor xmm6, xmm6, xmm6
    /* A later full-width write must cancel the pending upper zero. */
    vmovdqu ymm6, [rip + ones]
    vpxor xmm0, xmm0, xmm0
fault:
    xor eax, eax
    mov eax, [rax]
resume:
    vmovdqu [rip + result], ymm5
    vmovdqu [rip + result + 32], ymm8
    vmovdqu [rip + result + 64], ymm6
    cmp dword ptr [rip + caught], 1
    jne no_signal
    lea rsi, [rip + result]
    mov ecx, 2
check_zero:
    cmp qword ptr [rsi], -1
    jne bad_zero
    cmp qword ptr [rsi + 8], -1
    jne bad_zero
    cmp qword ptr [rsi + 16], 0
    jne bad_zero
    cmp qword ptr [rsi + 24], 0
    jne bad_zero
    add rsi, 32
    loop check_zero
    mov ecx, 4
check_ones:
    cmp qword ptr [rsi], -1
    jne bad_ones
    add rsi, 8
    loop check_ones
    xor edi, edi
    jmp exit
fail:
    mov edi, 1
    jmp exit
no_signal:
    mov edi, 2
    jmp exit
bad_zero:
    mov edi, 3
    jmp exit
bad_ones:
    mov edi, 4
exit:
    mov eax, 60
    syscall
handler:
    /* SA_SIGINFO: RDX points to ucontext; saved RIP is at offset 168. */
    lea rax, [rip + resume]
    mov [rdx + 168], rax
    inc dword ptr [rip + caught]
    ret
restorer:
    mov eax, 15
    syscall
.data
.balign 8
action:
    .quad handler, 0x04000004, restorer, 0
.balign 32
ones:
    .quad -1, -1, -1, -1
.bss
.balign 32
result: .skip 96
caught: .skip 4
.section .note.GNU-stack,"",@progbits
