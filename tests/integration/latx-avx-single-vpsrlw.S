/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSRLW offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsrlw_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsrlw_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsrlw_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsrlw_run
    .type latx_avx_single_vpsrlw_run, @function
latx_avx_single_vpsrlw_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsrlw_input_a]
    vpsrlw xmm2, xmm0, 0
    SAVE_VPSRLW 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlw_input_a]
    vpsrlw xmm0, xmm0, 14
    SAVE_VPSRLW 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrlw_input_a]
    vpsrlw xmm2, xmm0, 15
    SAVE_VPSRLW 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlw_input_a]
    vpsrlw xmm0, xmm0, 255
    SAVE_VPSRLW 96, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrlw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlw_count_0]
    vpsrlw xmm2, xmm0, xmm1
    SAVE_VPSRLW 128, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlw_input_a]
    vpsrlw xmm2, xmm0, xmmword ptr [rip + vpsrlw_count_14]
    SAVE_VPSRLW 160, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlw_count_15]
    vpsrlw xmm0, xmm0, xmm1
    SAVE_VPSRLW 192, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrlw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlw_count_255]
    vpsrlw xmm1, xmm0, xmm1
    SAVE_VPSRLW 224, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsrlw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlw_count_0]
    vpsrlw ymm2, ymm0, xmm1
    SAVE_VPSRLW 256, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrlw_input_a]
    vpsrlw ymm2, ymm0, xmmword ptr [rip + vpsrlw_count_14]
    SAVE_VPSRLW 288, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrlw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlw_count_15]
    vpsrlw ymm0, ymm0, xmm1
    SAVE_VPSRLW 320, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsrlw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlw_count_255]
    vpsrlw ymm1, ymm0, xmm1
    SAVE_VPSRLW 352, 1
    ret

    .section .data
    .balign 32
vpsrlw_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsrlw_count_0: .quad 0, 0, 0, 0
vpsrlw_count_14: .quad 14, 14, 14, 14
vpsrlw_count_15: .quad 15, 15, 15, 15
vpsrlw_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsrlw_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
