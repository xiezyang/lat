/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .equ DQU_MASK_ZERO, 128
    .equ DQU_MASK_ONE, 144
    .equ DQU_MASK_MIX, 160
    .equ DQU_DATA, 192
    .equ DQU_SEED, 224
    .equ DQU_STORE, 512

    .macro SAVE offset
    vmovdqu xmmword ptr [rdi], xmm0
    vpxor xmm15, xmm15, xmm15
    vmovdqu xmmword ptr [rdi + 16], xmm15
    vmovdqu xmm14, xmmword ptr [rsi + \offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm15, xmmword ptr [rsi + \offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm15
    add rdi, 64
    .endm

    .macro STORE mask_offset, data_offset, address_offset
    vmovdqu xmm0, xmmword ptr [rsi + \data_offset]
    vmovdqu xmm1, xmmword ptr [rsi + \mask_offset]
    lea rdi, [rsi + \address_offset]
    vmaskmovdqu xmm0, xmm1
    .endm

    .globl latx_avx_single_vmaskmovdqu_run
latx_avx_single_vmaskmovdqu_run:
    push r12
    mov r12, rdi

    STORE DQU_MASK_ZERO, DQU_DATA, DQU_STORE
    mov rdi, r12
    SAVE DQU_STORE

    STORE DQU_MASK_ONE, DQU_DATA, DQU_STORE + 64
    mov rdi, r12
    SAVE DQU_STORE + 64

    STORE DQU_MASK_MIX, DQU_DATA, DQU_STORE + 128 + 3
    mov rdi, r12
    SAVE DQU_STORE + 128 + 3

    vmovdqu xmm0, xmmword ptr [rsi + DQU_MASK_MIX]
    lea rdi, [rsi + DQU_STORE + 192]
    vmaskmovdqu xmm0, xmm0
    mov rdi, r12
    SAVE DQU_STORE + 192

    pop r12
    ret

    .globl latx_avx_single_vmaskmovdqu_fault_store
latx_avx_single_vmaskmovdqu_fault_store:
    vmovdqu xmm0, xmmword ptr [rsi]
    vmovdqu xmm1, xmmword ptr [rsi + 16]
    vmaskmovdqu xmm0, xmm1
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .note.GNU-stack,"",@progbits
