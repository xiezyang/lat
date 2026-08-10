/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_VGATHERDPD offset, dreg, mreg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vgatherdpd_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vgatherdpd_xsave_area + 160 + (\dreg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vgatherdpd_xsave_area + 576 + (\dreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    movdqu xmm14, xmmword ptr [rip + vgatherdpd_xsave_area + 160 + (\mreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 32], xmm14
    movdqu xmm14, xmmword ptr [rip + vgatherdpd_xsave_area + 576 + (\mreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 48], xmm14
    .endm

    .globl latx_avx_single_vgatherdpd_run
    .type latx_avx_single_vgatherdpd_run, @function
latx_avx_single_vgatherdpd_run:
    test rsi, rsi
    jnz .Lfault
    vmovdqu ymm0, ymmword ptr [rip + vgatherdpd_dest_seed]
    vmovdqu xmm1, xmmword ptr [rip + vgatherdpd_indices_pos_xmm_d]
    vmovdqu xmm2, xmmword ptr [rip + vgatherdpd_mask_all_xmm]
    lea r8, [rip + gather_base]
    vgatherdpd xmm0, qword ptr [r8 + xmm1 * 4], xmm2
    SAVE_VGATHERDPD 0, 0, 2
    vmovdqu ymm0, ymmword ptr [rip + vgatherdpd_dest_seed]
    vmovdqu xmm1, xmmword ptr [rip + vgatherdpd_indices_neg_xmm_d]
    vmovdqu xmm2, xmmword ptr [rip + vgatherdpd_mask_partial_xmm]
    lea r8, [rip + gather_base]
    vgatherdpd xmm0, qword ptr [r8 + xmm1 * 4], xmm2
    SAVE_VGATHERDPD 64, 0, 2
    vmovdqu ymm0, ymmword ptr [rip + vgatherdpd_dest_seed]
    vmovdqu xmm1, xmmword ptr [rip + vgatherdpd_indices_pos_ymm_d]
    vmovdqu ymm2, ymmword ptr [rip + vgatherdpd_mask_all_ymm]
    lea r8, [rip + gather_base]
    vgatherdpd ymm0, qword ptr [r8 + xmm1 * 4], ymm2
    SAVE_VGATHERDPD 128, 0, 2
    vmovdqu ymm0, ymmword ptr [rip + vgatherdpd_dest_seed]
    vmovdqu xmm1, xmmword ptr [rip + vgatherdpd_indices_neg_ymm_d]
    vmovdqu ymm2, ymmword ptr [rip + vgatherdpd_mask_partial_ymm]
    lea r8, [rip + gather_base]
    vgatherdpd ymm0, qword ptr [r8 + xmm1 * 4], ymm2
    SAVE_VGATHERDPD 192, 0, 2
    ret
.Lfault:
    vmovdqu xmm1, xmmword ptr [rip + vgatherdpd_indices_zero_xmm_d]
    vmovdqu ymm2, ymmword ptr [rip + vgatherdpd_mask_all_ymm]
    vgatherdpd ymm0, qword ptr [rsi + xmm1 * 4], ymm2
    ret

    .section .data
    .balign 32
vgatherdpd_dest_seed: .zero 32
vgatherdpd_indices_pos_xmm_d: .long 0,2,4,6
vgatherdpd_indices_neg_xmm_d: .long -1,-2,-3,-4
vgatherdpd_indices_pos_ymm_d: .long 0,2,4,6,8,10,12,14
vgatherdpd_indices_neg_ymm_d: .long -1,-2,-3,-4,-5,-6,-7,-8
vgatherdpd_indices_zero_xmm_d: .long 0,0,0,0
vgatherdpd_indices_zero_ymm_d: .long 0,0,0,0,0,0,0,0
vgatherdpd_mask_all_xmm: .long -1,-1,-1,-1
vgatherdpd_mask_partial_xmm: .long -1,0,-1,0
vgatherdpd_mask_all_ymm: .long -1,-1,-1,-1,-1,-1,-1,-1
vgatherdpd_mask_partial_ymm: .long -1,0,-1,0,-1,0,-1,0
    .balign 8
vgatherdpd_indices_pos_xmm_q: .quad 0,1
vgatherdpd_indices_neg_xmm_q: .quad -1,-2
vgatherdpd_indices_zero_xmm_q: .quad 0,0
vgatherdpd_indices_pos_ymm_q: .quad 0,1,2,3
vgatherdpd_indices_neg_ymm_q: .quad -1,-2,-3,-4
vgatherdpd_indices_zero_ymm_q: .quad 0,0,0,0
    .balign 64
vgatherdpd_xsave_area: .zero 4096
    .balign 32
gather_table:
    .rept 16
    .quad 0x3ff0000000000001
    .endr
gather_base:
    .rept 16
    .quad 0x3ff0000000000001
    .endr
