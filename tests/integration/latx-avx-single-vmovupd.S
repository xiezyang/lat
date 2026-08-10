/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .equ UPD_SRC, 0
    .equ UPD_SEED, 256
    .equ UPD_STORE, 512

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

    .globl latx_avx_single_vmovupd_run
latx_avx_single_vmovupd_run:
    vmovdqu xmm0, xmmword ptr [rsi + UPD_SEED]
    vmovupd xmm0, xmm0
    SAVE_XMM UPD_SEED

    vmovupd xmm0, xmmword ptr [rsi + UPD_SRC]
    SAVE_XMM UPD_SRC

    vmovdqu xmm0, xmmword ptr [rsi + UPD_SEED]
    vmovupd xmmword ptr [rsi + UPD_STORE], xmm0
    SAVE_XMM UPD_STORE

    vmovdqu ymm0, ymmword ptr [rsi + UPD_SEED]
    vmovupd ymm0, ymm0
    SAVE_YMM UPD_SEED

    vmovupd ymm0, ymmword ptr [rsi + UPD_SRC]
    SAVE_YMM UPD_SRC

    vmovdqu ymm0, ymmword ptr [rsi + UPD_SEED]
    vmovupd ymmword ptr [rsi + UPD_STORE + 32], ymm0
    SAVE_YMM UPD_STORE + 32

    vzeroupper
    ret

    .globl latx_avx_single_vmovupd_fault_xmm_load
latx_avx_single_vmovupd_fault_xmm_load:
    vmovupd xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovupd_fault_xmm_store
latx_avx_single_vmovupd_fault_xmm_store:
    vmovdqu xmm15, xmmword ptr [rip + vmovupd_fault_pattern]
    vmovupd xmmword ptr [rdi], xmm15
    ret

    .globl latx_avx_single_vmovupd_fault_ymm_load
latx_avx_single_vmovupd_fault_ymm_load:
    vmovupd ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovupd_fault_ymm_store
latx_avx_single_vmovupd_fault_ymm_store:
    vmovdqu ymm15, ymmword ptr [rip + vmovupd_fault_pattern]
    vmovupd ymmword ptr [rdi], ymm15
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .rodata
    .balign 32
vmovupd_fault_pattern:
    .quad 0x8877665544332211, 0x0f1e2d3c4b5a6978
    .quad 0x0706050403020100, 0x0f0e0d0c0b0a0908

    .section .note.GNU-stack,"",@progbits
