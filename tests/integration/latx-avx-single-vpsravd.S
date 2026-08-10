/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSRAVD offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsravd_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsravd_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsravd_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsravd_run
    .type latx_avx_single_vpsravd_run, @function
latx_avx_single_vpsravd_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsravd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsravd_count_0]
    vpsravd xmm2, xmm0, xmm1
    SAVE_VPSRAVD 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsravd_input_a]
    vpsravd xmm2, xmm0, xmmword ptr [rip + vpsravd_count_30]
    SAVE_VPSRAVD 32, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsravd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsravd_count_31]
    vpsravd xmm0, xmm0, xmm1
    SAVE_VPSRAVD 64, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsravd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsravd_count_255]
    vpsravd xmm1, xmm0, xmm1
    SAVE_VPSRAVD 96, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsravd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsravd_count_0]
    vpsravd ymm2, ymm0, ymm1
    SAVE_VPSRAVD 128, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsravd_input_a]
    vpsravd ymm2, ymm0, ymmword ptr [rip + vpsravd_count_30]
    SAVE_VPSRAVD 160, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsravd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsravd_count_31]
    vpsravd ymm0, ymm0, ymm1
    SAVE_VPSRAVD 192, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsravd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsravd_count_255]
    vpsravd ymm1, ymm0, ymm1
    SAVE_VPSRAVD 224, 1
    ret

    .section .data
    .balign 32
vpsravd_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsravd_count_0: .quad 0, 0, 0, 0
vpsravd_count_30: .quad 30, 30, 30, 30
vpsravd_count_31: .quad 31, 31, 31, 31
vpsravd_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsravd_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
