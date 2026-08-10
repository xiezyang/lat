/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .equ PS_SRC, 0
    .equ PS_MASK_ZERO, 256
    .equ PS_MASK_ONE, 288
    .equ PS_MASK_MIX, 320
    .equ PS_DATA, 384
    .equ PS_SEED, 448
    .equ PS_STORE_BASE, 1024

    .macro SAVE_XMM memory_offset
    vmovdqu xmmword ptr [rdi], xmm0
    vpxor xmm15, xmm15, xmm15
    vmovdqu xmmword ptr [rdi + 16], xmm15
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm15, xmmword ptr [rsi + \memory_offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm15
    add rdi, 64
    .endm

    .macro SAVE_YMM memory_offset
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu xmm14, xmmword ptr [rsi + \memory_offset]
    vmovdqu xmmword ptr [rdi + 32], xmm14
    vmovdqu xmm15, xmmword ptr [rsi + \memory_offset + 16]
    vmovdqu xmmword ptr [rdi + 48], xmm15
    add rdi, 64
    .endm

    .macro STORE_OFFSET case_no
    .if \case_no == 0
    .set STORE_ADDR, PS_STORE_BASE
    .elseif \case_no == 1
    .set STORE_ADDR, PS_STORE_BASE + 64
    .elseif \case_no == 2
    .set STORE_ADDR, PS_STORE_BASE + 128
    .elseif \case_no == 3
    .set STORE_ADDR, PS_STORE_BASE + 192
    .elseif \case_no == 4
    .set STORE_ADDR, PS_STORE_BASE + 256
    .else
    .set STORE_ADDR, PS_STORE_BASE + 320
    .endif
    .endm

    .globl latx_avx_single_vmaskmovps_run
latx_avx_single_vmaskmovps_run:
    push rbx
    push r12

    vmovdqu xmm0, xmmword ptr [rsi + PS_SEED]
    vmovdqu xmm1, xmmword ptr [rsi + PS_MASK_ZERO]
    vmaskmovps xmm0, xmm1, xmmword ptr [rsi + PS_SRC]
    SAVE_XMM PS_SRC

    vmovdqu xmm0, xmmword ptr [rsi + PS_SEED]
    vmovdqu xmm1, xmmword ptr [rsi + PS_MASK_ONE]
    vmaskmovps xmm0, xmm1, xmmword ptr [rsi + PS_SRC]
    SAVE_XMM PS_SRC

    vmovdqu xmm0, xmmword ptr [rsi + PS_SEED]
    vmovdqu xmm1, xmmword ptr [rsi + PS_MASK_MIX]
    vmaskmovps xmm0, xmm1, xmmword ptr [rsi + PS_SRC + 3]
    SAVE_XMM PS_SRC + 3

    vmovdqu xmm0, xmmword ptr [rsi + PS_MASK_MIX]
    vmaskmovps xmm0, xmm0, xmmword ptr [rsi + PS_SRC]
    SAVE_XMM PS_SRC

    vmovdqu ymm0, ymmword ptr [rsi + PS_SEED]
    vmovdqu ymm1, ymmword ptr [rsi + PS_MASK_MIX]
    vmaskmovps ymm0, ymm1, ymmword ptr [rsi + PS_SRC]
    SAVE_YMM PS_SRC

    vmovdqu ymm0, ymmword ptr [rsi + PS_SEED]
    vmovdqu ymm1, ymmword ptr [rsi + PS_MASK_ONE]
    vmaskmovps ymm0, ymm1, ymmword ptr [rsi + PS_SRC + 5]
    SAVE_YMM PS_SRC + 5

    STORE_OFFSET 0
    vmovdqu xmm0, xmmword ptr [rsi + PS_DATA]
    vmovdqu xmm1, xmmword ptr [rsi + PS_MASK_ZERO]
    vmaskmovps xmmword ptr [rsi + STORE_ADDR], xmm1, xmm0
    SAVE_XMM STORE_ADDR

    STORE_OFFSET 1
    vmovdqu xmm0, xmmword ptr [rsi + PS_DATA]
    vmovdqu xmm1, xmmword ptr [rsi + PS_MASK_ONE]
    vmaskmovps xmmword ptr [rsi + STORE_ADDR], xmm1, xmm0
    SAVE_XMM STORE_ADDR

    STORE_OFFSET 2
    vmovdqu xmm0, xmmword ptr [rsi + PS_DATA]
    vmovdqu xmm1, xmmword ptr [rsi + PS_MASK_MIX]
    vmaskmovps xmmword ptr [rsi + STORE_ADDR + 3], xmm1, xmm0
    SAVE_XMM STORE_ADDR + 3

    STORE_OFFSET 3
    vmovdqu xmm0, xmmword ptr [rsi + PS_MASK_MIX]
    vmaskmovps xmmword ptr [rsi + STORE_ADDR], xmm0, xmm0
    SAVE_XMM STORE_ADDR

    STORE_OFFSET 4
    vmovdqu ymm0, ymmword ptr [rsi + PS_DATA]
    vmovdqu ymm1, ymmword ptr [rsi + PS_MASK_MIX]
    vmaskmovps ymmword ptr [rsi + STORE_ADDR + 5], ymm1, ymm0
    SAVE_YMM STORE_ADDR + 5

    STORE_OFFSET 5
    vmovdqu ymm0, ymmword ptr [rsi + PS_DATA]
    vmovdqu ymm1, ymmword ptr [rsi + PS_MASK_ONE]
    vmaskmovps ymmword ptr [rsi + STORE_ADDR], ymm1, ymm0
    SAVE_YMM STORE_ADDR

    vzeroupper
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vmaskmovps_fault_load
latx_avx_single_vmaskmovps_fault_load:
    vmovdqu xmm1, xmmword ptr [rsi]
    vmaskmovps xmm0, xmm1, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmaskmovps_fault_store
latx_avx_single_vmaskmovps_fault_store:
    vmovdqu xmm0, xmmword ptr [rsi]
    vmaskmovps xmmword ptr [rdi], xmm0, xmm0
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .note.GNU-stack,"",@progbits
