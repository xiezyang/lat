/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSRAW offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsraw_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsraw_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsraw_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsraw_run
    .type latx_avx_single_vpsraw_run, @function
latx_avx_single_vpsraw_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsraw_input_a]
    vpsraw xmm2, xmm0, 0
    SAVE_VPSRAW 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsraw_input_a]
    vpsraw xmm0, xmm0, 14
    SAVE_VPSRAW 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsraw_input_a]
    vpsraw xmm2, xmm0, 15
    SAVE_VPSRAW 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsraw_input_a]
    vpsraw xmm0, xmm0, 255
    SAVE_VPSRAW 96, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsraw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsraw_count_0]
    vpsraw xmm2, xmm0, xmm1
    SAVE_VPSRAW 128, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsraw_input_a]
    vpsraw xmm2, xmm0, xmmword ptr [rip + vpsraw_count_14]
    SAVE_VPSRAW 160, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsraw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsraw_count_15]
    vpsraw xmm0, xmm0, xmm1
    SAVE_VPSRAW 192, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsraw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsraw_count_255]
    vpsraw xmm1, xmm0, xmm1
    SAVE_VPSRAW 224, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsraw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsraw_count_0]
    vpsraw ymm2, ymm0, xmm1
    SAVE_VPSRAW 256, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsraw_input_a]
    vpsraw ymm2, ymm0, xmmword ptr [rip + vpsraw_count_14]
    SAVE_VPSRAW 288, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsraw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsraw_count_15]
    vpsraw ymm0, ymm0, xmm1
    SAVE_VPSRAW 320, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsraw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsraw_count_255]
    vpsraw ymm1, ymm0, xmm1
    SAVE_VPSRAW 352, 1
    ret

    .section .data
    .balign 32
vpsraw_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsraw_count_0: .quad 0, 0, 0, 0
vpsraw_count_14: .quad 14, 14, 14, 14
vpsraw_count_15: .quad 15, 15, 15, 15
vpsraw_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsraw_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
