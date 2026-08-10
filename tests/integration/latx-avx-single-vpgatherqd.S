/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_VPGATHERQD offset, dreg, mreg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpgatherqd_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpgatherqd_xsave_area + 160 + (\dreg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpgatherqd_xsave_area + 576 + (\dreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    movdqu xmm14, xmmword ptr [rip + vpgatherqd_xsave_area + 160 + (\mreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 32], xmm14
    movdqu xmm14, xmmword ptr [rip + vpgatherqd_xsave_area + 576 + (\mreg) * 16]
    movdqu xmmword ptr [rdi + \offset + 48], xmm14
    .endm

    .globl latx_avx_single_vpgatherqd_run
    .type latx_avx_single_vpgatherqd_run, @function
latx_avx_single_vpgatherqd_run:
    test rsi, rsi
    jnz .Lfault
    vmovdqu ymm0, ymmword ptr [rip + vpgatherqd_dest_seed]
    vmovdqu xmm1, xmmword ptr [rip + vpgatherqd_indices_pos_xmm_q]
    vmovdqu xmm2, xmmword ptr [rip + vpgatherqd_mask_all_xmm]
    lea r8, [rip + gather_base]
    vpgatherqd xmm0, dword ptr [r8 + xmm1 * 8], xmm2
    SAVE_VPGATHERQD 0, 0, 2
    vmovdqu ymm0, ymmword ptr [rip + vpgatherqd_dest_seed]
    vmovdqu xmm1, xmmword ptr [rip + vpgatherqd_indices_neg_xmm_q]
    vmovdqu xmm2, xmmword ptr [rip + vpgatherqd_mask_partial_xmm]
    lea r8, [rip + gather_base]
    vpgatherqd xmm0, dword ptr [r8 + xmm1 * 8], xmm2
    SAVE_VPGATHERQD 64, 0, 2
    ret
.Lfault:
    vmovdqu xmm1, xmmword ptr [rip + vpgatherqd_indices_zero_xmm_q]
    vmovdqu xmm2, xmmword ptr [rip + vpgatherqd_mask_all_xmm]
    vpgatherqd xmm0, dword ptr [rsi + xmm1 * 8], xmm2
    ret

    .section .data
    .balign 32
vpgatherqd_dest_seed: .zero 32
vpgatherqd_indices_pos_xmm_d: .long 0,2,4,6
vpgatherqd_indices_neg_xmm_d: .long -1,-2,-3,-4
vpgatherqd_indices_pos_ymm_d: .long 0,2,4,6,8,10,12,14
vpgatherqd_indices_neg_ymm_d: .long -1,-2,-3,-4,-5,-6,-7,-8
vpgatherqd_indices_zero_xmm_d: .long 0,0,0,0
vpgatherqd_indices_zero_ymm_d: .long 0,0,0,0,0,0,0,0
vpgatherqd_mask_all_xmm: .long -1,-1,-1,-1
vpgatherqd_mask_partial_xmm: .long -1,0,-1,0
vpgatherqd_mask_all_ymm: .long -1,-1,-1,-1,-1,-1,-1,-1
vpgatherqd_mask_partial_ymm: .long -1,0,-1,0,-1,0,-1,0
    .balign 8
vpgatherqd_indices_pos_xmm_q: .quad 0,1
vpgatherqd_indices_neg_xmm_q: .quad -1,-2
vpgatherqd_indices_zero_xmm_q: .quad 0,0
vpgatherqd_indices_pos_ymm_q: .quad 0,1,2,3
vpgatherqd_indices_neg_ymm_q: .quad -1,-2,-3,-4
vpgatherqd_indices_zero_ymm_q: .quad 0,0,0,0
    .balign 64
vpgatherqd_xsave_area: .zero 4096
    .balign 32
gather_table:
    .rept 16
    .quad 0x3ff0000000000001
    .endr
gather_base:
    .rept 16
    .quad 0x3ff0000000000001
    .endr
