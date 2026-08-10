/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .equ DQA_SRC, 0
    .equ DQA_SEED, 256
    .equ DQA_STORE, 512

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

    .globl latx_avx_single_vmovdqa_run
latx_avx_single_vmovdqa_run:
    vmovdqu xmm0, xmmword ptr [rsi + DQA_SEED]
    vmovdqa xmm0, xmm0
    SAVE_XMM DQA_SEED

    vmovdqa xmm0, xmmword ptr [rsi + DQA_SRC]
    SAVE_XMM DQA_SRC

    vmovdqu xmm0, xmmword ptr [rsi + DQA_SEED]
    vmovdqa xmmword ptr [rsi + DQA_STORE], xmm0
    vmovdqu xmm0, xmmword ptr [rsi + DQA_STORE]
    SAVE_XMM DQA_STORE

    vmovdqu ymm0, ymmword ptr [rsi + DQA_SEED]
    vmovdqa ymm0, ymm0
    SAVE_YMM DQA_SEED

    vmovdqa ymm0, ymmword ptr [rsi + DQA_SRC]
    SAVE_YMM DQA_SRC

    vmovdqu ymm0, ymmword ptr [rsi + DQA_SEED]
    vmovdqa ymmword ptr [rsi + DQA_STORE + 32], ymm0
    vmovdqu ymm0, ymmword ptr [rsi + DQA_STORE + 32]
    SAVE_YMM DQA_STORE + 32

    vzeroupper
    ret

    .globl latx_avx_single_vmovdqa_fault_xmm_load
latx_avx_single_vmovdqa_fault_xmm_load:
    vmovdqa xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovdqa_fault_xmm_store
latx_avx_single_vmovdqa_fault_xmm_store:
    vmovdqu xmm15, xmmword ptr [rip + vmovdqa_fault_pattern]
    vmovdqa xmmword ptr [rdi], xmm15
    ret

    .globl latx_avx_single_vmovdqa_fault_ymm_load
latx_avx_single_vmovdqa_fault_ymm_load:
    vmovdqa ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovdqa_fault_ymm_store
latx_avx_single_vmovdqa_fault_ymm_store:
    vmovdqu ymm15, ymmword ptr [rip + vmovdqa_fault_pattern]
    vmovdqa ymmword ptr [rdi], ymm15
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .rodata
    .balign 32
vmovdqa_fault_pattern:
    .quad 0x8877665544332211, 0x0f1e2d3c4b5a6978
    .quad 0x0706050403020100, 0x0f0e0d0c0b1a1918

    .section .note.GNU-stack,"",@progbits
