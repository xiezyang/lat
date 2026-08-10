/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix

    .data
    .balign 16
    .globl latx_avx_vmovq_rip_input
latx_avx_vmovq_rip_input:
    .quad 0
    .balign 16
    .globl latx_avx_vmovq_rip_store
latx_avx_vmovq_rip_store:
    .fill 16, 1, 0x5a

    .text
    .globl latx_avx_single_vmovq
    .type latx_avx_single_vmovq, @function
latx_avx_single_vmovq:
    push r15
    mov r10, rcx
    mov r11, r8

    pcmpeqd xmm0, xmm0
    mov rax, rdi
    vmovq xmm0, rax
    movdqu xmmword ptr [r10 + 0], xmm0

    pcmpeqd xmm15, xmm15
    mov r15, rdi
    vmovq xmm15, r15
    movdqu xmmword ptr [r10 + 16], xmm15

    movq xmm0, rdi
    pcmpeqd xmm1, xmm1
    punpcklqdq xmm0, xmm1
    mov rax, -1
    vmovq rax, xmm0
    mov qword ptr [r10 + 32], rax

    movq xmm15, rdi
    pcmpeqd xmm1, xmm1
    punpcklqdq xmm15, xmm1
    mov r15, -1
    vmovq r15, xmm15
    mov qword ptr [r10 + 48], r15

    movq xmm0, rdi
    pcmpeqd xmm1, xmm1
    punpcklqdq xmm0, xmm1
    vmovq xmm0, xmm0
    movdqu xmmword ptr [r10 + 64], xmm0

    movq xmm0, rdi
    pcmpeqd xmm1, xmm1
    punpcklqdq xmm0, xmm1
    pcmpeqd xmm15, xmm15
    vmovq xmm15, xmm0
    movdqu xmmword ptr [r10 + 80], xmm15

    movq xmm1, rdi
    pcmpeqd xmm2, xmm2
    punpcklqdq xmm1, xmm2
    pcmpeqd xmm0, xmm0
    /* VEX.128.66.0F.W1 D6 /r: vmovq xmm0, xmm1. */
    .byte 0xc4, 0xe1, 0xf9, 0xd6, 0xc8
    movdqu xmmword ptr [r10 + 96], xmm0

    pcmpeqd xmm0, xmm0
    vmovq xmm0, qword ptr [rsi]
    movdqu xmmword ptr [r10 + 112], xmm0

    pcmpeqd xmm15, xmm15
    mov eax, 2
    vmovq xmm15, qword ptr [rsi + rax * 8 + 8]
    movdqu xmmword ptr [r10 + 128], xmm15

    pcmpeqd xmm0, xmm0
    vmovq xmm0, qword ptr [rsi + 120]
    movdqu xmmword ptr [r10 + 144], xmm0

    pcmpeqd xmm15, xmm15
    vmovq xmm15, qword ptr [rsi + 4096]
    movdqu xmmword ptr [r10 + 160], xmm15

    pcmpeqd xmm0, xmm0
    vmovq xmm0, qword ptr [rip + latx_avx_vmovq_rip_input]
    movdqu xmmword ptr [r10 + 176], xmm0

    pcmpeqd xmm15, xmm15
    lea rax, [rsi + 257]
    vmovq xmm15, qword ptr [rax]
    movdqu xmmword ptr [r10 + 192], xmm15

    pcmpeqd xmm0, xmm0
    vmovq xmm0, qword ptr [rdx]
    movdqu xmmword ptr [r10 + 208], xmm0

    movq xmm0, rdi
    movq xmm15, rdi
    vmovq qword ptr [r11], xmm0
    mov eax, 2
    vmovq qword ptr [r11 + rax * 8 + 48], xmm15
    vmovq qword ptr [r11 + 120], xmm0
    vmovq qword ptr [r11 + 4096], xmm15
    lea rax, [r11 + 257]
    vmovq qword ptr [rax], xmm0
    vmovq qword ptr [rip + latx_avx_vmovq_rip_store], xmm15
    vmovq qword ptr [r9], xmm0

    pop r15
    ret

    .globl latx_avx_single_vmovq_fault_load
    .type latx_avx_single_vmovq_fault_load, @function
latx_avx_single_vmovq_fault_load:
    pcmpeqd xmm0, xmm0
    vmovq xmm0, qword ptr [rdi]
    ret

    .globl latx_avx_single_vmovq_fault_store
    .type latx_avx_single_vmovq_fault_store, @function
latx_avx_single_vmovq_fault_store:
    movq xmm0, rsi
    vmovq qword ptr [rdi], xmm0
    ret

    .section .note.GNU-stack,"",@progbits
