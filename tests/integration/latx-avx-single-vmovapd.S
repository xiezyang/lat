/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .equ APD_SRC, 0
    .equ APD_SEED, 256
    .equ APD_STORE, 512

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

    .globl latx_avx_single_vmovapd_run
latx_avx_single_vmovapd_run:
    vmovdqu xmm0, xmmword ptr [rsi + APD_SEED]
    vmovapd xmm0, xmm0
    SAVE_XMM APD_SEED

    vmovapd xmm0, xmmword ptr [rsi + APD_SRC]
    SAVE_XMM APD_SRC

    vmovdqu xmm0, xmmword ptr [rsi + APD_SEED]
    vmovapd xmmword ptr [rsi + APD_STORE], xmm0
    SAVE_XMM APD_STORE

    vmovdqu ymm0, ymmword ptr [rsi + APD_SEED]
    vmovapd ymm0, ymm0
    SAVE_YMM APD_SEED

    vmovapd ymm0, ymmword ptr [rsi + APD_SRC]
    SAVE_YMM APD_SRC

    vmovdqu ymm0, ymmword ptr [rsi + APD_SEED]
    vmovapd ymmword ptr [rsi + APD_STORE + 32], ymm0
    SAVE_YMM APD_STORE + 32

    vzeroupper
    ret

    .globl latx_avx_single_vmovapd_fault_xmm_load
latx_avx_single_vmovapd_fault_xmm_load:
    vmovapd xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovapd_fault_xmm_store
latx_avx_single_vmovapd_fault_xmm_store:
    vmovdqu xmm15, xmmword ptr [rip + vmovapd_fault_pattern]
    vmovapd xmmword ptr [rdi], xmm15
    ret

    .globl latx_avx_single_vmovapd_fault_ymm_load
latx_avx_single_vmovapd_fault_ymm_load:
    vmovapd ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovapd_fault_ymm_store
latx_avx_single_vmovapd_fault_ymm_store:
    vmovdqu ymm15, ymmword ptr [rip + vmovapd_fault_pattern]
    vmovapd ymmword ptr [rdi], ymm15
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .rodata
    .balign 32
vmovapd_fault_pattern:
    .quad 0x8877665544332211, 0x0f1e2d3c4b5a6978
    .quad 0x0706050403020100, 0x0f0e0d0c0b0a0908

    .section .note.GNU-stack,"",@progbits
