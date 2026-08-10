/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .equ SRC, 0
    .equ MASK_ZERO, 128
    .equ MASK_ONE, 160
    .equ MASK_MIX, 192
    .equ MASK_FAULT_MIX, 224
    .equ DATA, 256
    .equ STORE_BASE, 1024

    .macro SAVE_XMM memory_offset
    vmovdqu xmmword ptr [rdi], xmm0
    vpxor xmm15, xmm15, xmm15
    vmovdqu xmmword ptr [rdi + 16], xmm15
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm14
    add rdi, 64
    .endm

    .macro SAVE_YMM memory_offset
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm14
    add rdi, 64
    .endm

    .globl latx_avx_single_vpmaskmovq_run
latx_avx_single_vpmaskmovq_run:
    push r12
    xor r8d, r8d

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_ZERO]
    vpmaskmovq xmm0, xmm1, xmmword ptr [rsi + SRC]
    SAVE_XMM SRC

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_ONE]
    vpmaskmovq xmm0, xmm1, xmmword ptr [rsi + SRC]
    SAVE_XMM SRC

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_MIX]
    vpmaskmovq xmm0, xmm1, xmmword ptr [rsi + SRC + 3]
    SAVE_XMM SRC + 3

    vmovdqu xmm0, xmmword ptr [rsi + MASK_MIX]
    vpmaskmovq xmm0, xmm0, xmmword ptr [rsi + SRC]
    SAVE_XMM SRC

    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rsi + MASK_MIX]
    vpmaskmovq ymm0, ymm1, ymmword ptr [rsi + SRC]
    SAVE_YMM SRC

    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rsi + MASK_ONE]
    vpmaskmovq ymm0, ymm1, ymmword ptr [rsi + r8*8 + SRC + 5]
    SAVE_YMM SRC + 5

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_ZERO]
    vpmaskmovq xmmword ptr [rsi + STORE_BASE], xmm1, xmm0
    SAVE_XMM STORE_BASE

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_ONE]
    vpmaskmovq xmmword ptr [rsi + STORE_BASE + 64], xmm1, xmm0
    SAVE_XMM STORE_BASE + 64

    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rsi + MASK_MIX]
    vpmaskmovq xmmword ptr [rsi + STORE_BASE + 128 + 3], xmm1, xmm0
    SAVE_XMM STORE_BASE + 128 + 3

    vmovdqu xmm0, xmmword ptr [rsi + MASK_MIX]
    vpmaskmovq xmmword ptr [rsi + STORE_BASE + 192], xmm0, xmm0
    SAVE_XMM STORE_BASE + 192

    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rsi + MASK_MIX]
    vpmaskmovq ymmword ptr [rsi + r8*8 + STORE_BASE + 256 + 5], ymm1, ymm0
    SAVE_YMM STORE_BASE + 256 + 5

    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rsi + MASK_ONE]
    vpmaskmovq ymmword ptr [rsi + r8*8 + STORE_BASE + 320], ymm1, ymm0
    SAVE_YMM STORE_BASE + 320

    pop r12
    vzeroupper
    ret

    .globl latx_avx_single_vpmaskmovq_fault_load
latx_avx_single_vpmaskmovq_fault_load:
    vmovdqu xmm1, xmmword ptr [rdx]
    vpmaskmovq xmm0, xmm1, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vpmaskmovq_fault_load_ymm
latx_avx_single_vpmaskmovq_fault_load_ymm:
    vmovdqu ymm1, ymmword ptr [rdx]
    vpmaskmovq ymm0, ymm1, ymmword ptr [rdi]
    vzeroupper
    ret

    .globl latx_avx_single_vpmaskmovq_fault_store
latx_avx_single_vpmaskmovq_fault_store:
    vmovdqu xmm0, xmmword ptr [rsi + DATA]
    vmovdqu xmm1, xmmword ptr [rdx]
    vpmaskmovq xmmword ptr [rdi], xmm1, xmm0
    ret

    .globl latx_avx_single_vpmaskmovq_fault_store_ymm
latx_avx_single_vpmaskmovq_fault_store_ymm:
    vmovdqu ymm0, ymmword ptr [rsi + DATA]
    vmovdqu ymm1, ymmword ptr [rdx]
    vpmaskmovq ymmword ptr [rdi], ymm1, ymm0
    vzeroupper
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .note.GNU-stack,"",@progbits
