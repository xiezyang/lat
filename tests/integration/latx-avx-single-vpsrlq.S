/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSRLQ offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsrlq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsrlq_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsrlq_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsrlq_run
    .type latx_avx_single_vpsrlq_run, @function
latx_avx_single_vpsrlq_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsrlq_input_a]
    vpsrlq xmm2, xmm0, 0
    SAVE_VPSRLQ 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlq_input_a]
    vpsrlq xmm0, xmm0, 62
    SAVE_VPSRLQ 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrlq_input_a]
    vpsrlq xmm2, xmm0, 63
    SAVE_VPSRLQ 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlq_input_a]
    vpsrlq xmm0, xmm0, 255
    SAVE_VPSRLQ 96, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrlq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlq_count_0]
    vpsrlq xmm2, xmm0, xmm1
    SAVE_VPSRLQ 128, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlq_input_a]
    vpsrlq xmm2, xmm0, xmmword ptr [rip + vpsrlq_count_62]
    SAVE_VPSRLQ 160, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlq_count_63]
    vpsrlq xmm0, xmm0, xmm1
    SAVE_VPSRLQ 192, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrlq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlq_count_255]
    vpsrlq xmm1, xmm0, xmm1
    SAVE_VPSRLQ 224, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsrlq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlq_count_0]
    vpsrlq ymm2, ymm0, xmm1
    SAVE_VPSRLQ 256, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrlq_input_a]
    vpsrlq ymm2, ymm0, xmmword ptr [rip + vpsrlq_count_62]
    SAVE_VPSRLQ 288, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrlq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlq_count_63]
    vpsrlq ymm0, ymm0, xmm1
    SAVE_VPSRLQ 320, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsrlq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlq_count_255]
    vpsrlq ymm1, ymm0, xmm1
    SAVE_VPSRLQ 352, 1
    ret

    .section .data
    .balign 32
vpsrlq_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsrlq_count_0: .quad 0, 0, 0, 0
vpsrlq_count_62: .quad 62, 62, 62, 62
vpsrlq_count_63: .quad 63, 63, 63, 63
vpsrlq_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsrlq_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
