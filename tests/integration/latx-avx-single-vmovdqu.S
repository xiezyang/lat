/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .equ DQU_SRC, 0
    .equ DQU_SEED, 256
    .equ DQU_STORE, 512

    .macro SAVE_XMM offset
    vmovdqu xmmword ptr [rdi], xmm0
    vpxor xmm15, xmm15, xmm15
    vmovdqu xmmword ptr [rdi + 16], xmm15
    vmovdqu xmm14, xmmword ptr [rsi + \offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm15, xmmword ptr [rsi + \offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm15
    add rdi, 64
    .endm

    .macro SAVE_YMM offset
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu xmm14, xmmword ptr [rsi + \offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm15, xmmword ptr [rsi + \offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm15
    add rdi, 64
    .endm

    .globl latx_avx_single_vmovdqu_run
latx_avx_single_vmovdqu_run:
    vmovdqu xmm0, xmmword ptr [rsi + DQU_SEED]
    vmovdqu xmm0, xmm0
    SAVE_XMM DQU_SEED

    vmovdqu xmm0, xmmword ptr [rsi + DQU_SRC + 1]
    SAVE_XMM DQU_SRC + 1

    vmovdqu xmm0, xmmword ptr [rsi + DQU_SEED]
    vmovdqu xmmword ptr [rsi + DQU_STORE + 1], xmm0
    vmovdqu xmm0, xmmword ptr [rsi + DQU_STORE + 1]
    SAVE_XMM DQU_STORE + 1

    vmovdqu ymm0, ymmword ptr [rsi + DQU_SEED]
    vmovdqu ymm0, ymm0
    SAVE_YMM DQU_SEED

    vmovdqu ymm0, ymmword ptr [rsi + DQU_SRC + 1]
    SAVE_YMM DQU_SRC + 1

    vmovdqu ymm0, ymmword ptr [rsi + DQU_SEED]
    vmovdqu ymmword ptr [rsi + DQU_STORE + 33], ymm0
    vmovdqu ymm0, ymmword ptr [rsi + DQU_STORE + 33]
    SAVE_YMM DQU_STORE + 33

    vzeroupper
    ret

    .globl latx_avx_single_vmovdqu_fault_xmm_load
latx_avx_single_vmovdqu_fault_xmm_load:
    vmovdqu xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovdqu_fault_xmm_store
latx_avx_single_vmovdqu_fault_xmm_store:
    vmovdqu xmm15, xmmword ptr [rip + vmovdqu_fault_pattern]
    vmovdqu xmmword ptr [rdi], xmm15
    ret

    .globl latx_avx_single_vmovdqu_fault_ymm_load
latx_avx_single_vmovdqu_fault_ymm_load:
    vmovdqu ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovdqu_fault_ymm_store
latx_avx_single_vmovdqu_fault_ymm_store:
    vmovdqu ymm15, ymmword ptr [rip + vmovdqu_fault_pattern]
    vmovdqu ymmword ptr [rdi], ymm15
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .rodata
    .balign 32
vmovdqu_fault_pattern:
    .quad 0x8877665544332211, 0x0f1e2d3c4b5a6978
    .quad 0x0706050403020100, 0x0f0e0d0c0b1a1918

    .section .note.GNU-stack,"",@progbits
