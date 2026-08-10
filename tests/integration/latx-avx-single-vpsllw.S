/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSLLW offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsllw_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsllw_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsllw_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsllw_run
    .type latx_avx_single_vpsllw_run, @function
latx_avx_single_vpsllw_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsllw_input_a]
    vpsllw xmm2, xmm0, 0
    SAVE_VPSLLW 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllw_input_a]
    vpsllw xmm0, xmm0, 14
    SAVE_VPSLLW 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsllw_input_a]
    vpsllw xmm2, xmm0, 15
    SAVE_VPSLLW 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllw_input_a]
    vpsllw xmm0, xmm0, 255
    SAVE_VPSLLW 96, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsllw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllw_count_0]
    vpsllw xmm2, xmm0, xmm1
    SAVE_VPSLLW 128, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllw_input_a]
    vpsllw xmm2, xmm0, xmmword ptr [rip + vpsllw_count_14]
    SAVE_VPSLLW 160, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllw_count_15]
    vpsllw xmm0, xmm0, xmm1
    SAVE_VPSLLW 192, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsllw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllw_count_255]
    vpsllw xmm1, xmm0, xmm1
    SAVE_VPSLLW 224, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsllw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllw_count_0]
    vpsllw ymm2, ymm0, xmm1
    SAVE_VPSLLW 256, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsllw_input_a]
    vpsllw ymm2, ymm0, xmmword ptr [rip + vpsllw_count_14]
    SAVE_VPSLLW 288, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsllw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllw_count_15]
    vpsllw ymm0, ymm0, xmm1
    SAVE_VPSLLW 320, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsllw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllw_count_255]
    vpsllw ymm1, ymm0, xmm1
    SAVE_VPSLLW 352, 1
    ret

    .section .data
    .balign 32
vpsllw_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsllw_count_0: .quad 0, 0, 0, 0
vpsllw_count_14: .quad 14, 14, 14, 14
vpsllw_count_15: .quad 15, 15, 15, 15
vpsllw_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsllw_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
