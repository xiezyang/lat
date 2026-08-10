/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_VGATHERQPD offset, dreg, mreg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vgatherqpd_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vgatherqpd_xsave_area + 160 + (\dreg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vgatherqpd_xsave_area + 576 + (\dreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    movdqu xmm14, xmmword ptr [rip + vgatherqpd_xsave_area + 160 + (\mreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 32], xmm14
    movdqu xmm14, xmmword ptr [rip + vgatherqpd_xsave_area + 576 + (\mreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 48], xmm14
    .endm

    .globl latx_avx_single_vgatherqpd_run
    .type latx_avx_single_vgatherqpd_run, @function
latx_avx_single_vgatherqpd_run:
    test rsi, rsi
    jnz .Lfault
    vmovdqu ymm0, ymmword ptr [rip + vgatherqpd_dest_seed]
    vmovdqu xmm1, xmmword ptr [rip + vgatherqpd_indices_pos_xmm_q]
    vmovdqu xmm2, xmmword ptr [rip + vgatherqpd_mask_all_xmm]
    lea r8, [rip + gather_base]
    vgatherqpd xmm0, qword ptr [r8 + xmm1 * 8], xmm2
    SAVE_VGATHERQPD 0, 0, 2
    vmovdqu ymm0, ymmword ptr [rip + vgatherqpd_dest_seed]
    vmovdqu xmm1, xmmword ptr [rip + vgatherqpd_indices_neg_xmm_q]
    vmovdqu xmm2, xmmword ptr [rip + vgatherqpd_mask_partial_xmm]
    lea r8, [rip + gather_base]
    vgatherqpd xmm0, qword ptr [r8 + xmm1 * 8], xmm2
    SAVE_VGATHERQPD 64, 0, 2
    vmovdqu ymm0, ymmword ptr [rip + vgatherqpd_dest_seed]
    vmovdqu ymm1, ymmword ptr [rip + vgatherqpd_indices_pos_ymm_q]
    vmovdqu ymm2, ymmword ptr [rip + vgatherqpd_mask_all_ymm]
    lea r8, [rip + gather_base]
    vgatherqpd ymm0, qword ptr [r8 + ymm1 * 8], ymm2
    SAVE_VGATHERQPD 128, 0, 2
    vmovdqu ymm0, ymmword ptr [rip + vgatherqpd_dest_seed]
    vmovdqu ymm1, ymmword ptr [rip + vgatherqpd_indices_neg_ymm_q]
    vmovdqu ymm2, ymmword ptr [rip + vgatherqpd_mask_partial_ymm]
    lea r8, [rip + gather_base]
    vgatherqpd ymm0, qword ptr [r8 + ymm1 * 8], ymm2
    SAVE_VGATHERQPD 192, 0, 2
    ret
.Lfault:
    vmovdqu ymm1, ymmword ptr [rip + vgatherqpd_indices_zero_ymm_q]
    vmovdqu ymm2, ymmword ptr [rip + vgatherqpd_mask_all_ymm]
    vgatherqpd ymm0, qword ptr [rsi + ymm1 * 8], ymm2
    ret

    .section .data
    .balign 32
vgatherqpd_dest_seed: .zero 32
vgatherqpd_indices_pos_xmm_d: .long 0,2,4,6
vgatherqpd_indices_neg_xmm_d: .long -1,-2,-3,-4
vgatherqpd_indices_pos_ymm_d: .long 0,2,4,6,8,10,12,14
vgatherqpd_indices_neg_ymm_d: .long -1,-2,-3,-4,-5,-6,-7,-8
vgatherqpd_indices_zero_xmm_d: .long 0,0,0,0
vgatherqpd_indices_zero_ymm_d: .long 0,0,0,0,0,0,0,0
vgatherqpd_mask_all_xmm: .long -1,-1,-1,-1
vgatherqpd_mask_partial_xmm: .long -1,0,-1,0
vgatherqpd_mask_all_ymm: .long -1,-1,-1,-1,-1,-1,-1,-1
vgatherqpd_mask_partial_ymm: .long -1,0,-1,0,-1,0,-1,0
    .balign 8
vgatherqpd_indices_pos_xmm_q: .quad 0,1
vgatherqpd_indices_neg_xmm_q: .quad -1,-2
vgatherqpd_indices_zero_xmm_q: .quad 0,0
vgatherqpd_indices_pos_ymm_q: .quad 0,1,2,3
vgatherqpd_indices_neg_ymm_q: .quad -1,-2,-3,-4
vgatherqpd_indices_zero_ymm_q: .quad 0,0,0,0
    .balign 64
vgatherqpd_xsave_area: .zero 4096
    .balign 32
gather_table:
    .rept 16
    .quad 0x3ff0000000000001
    .endr
gather_base:
    .rept 16
    .quad 0x3ff0000000000001
    .endr
