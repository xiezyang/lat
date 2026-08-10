/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSLLQ offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsllq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsllq_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsllq_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsllq_run
    .type latx_avx_single_vpsllq_run, @function
latx_avx_single_vpsllq_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsllq_input_a]
    vpsllq xmm2, xmm0, 0
    SAVE_VPSLLQ 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllq_input_a]
    vpsllq xmm0, xmm0, 62
    SAVE_VPSLLQ 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsllq_input_a]
    vpsllq xmm2, xmm0, 63
    SAVE_VPSLLQ 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllq_input_a]
    vpsllq xmm0, xmm0, 255
    SAVE_VPSLLQ 96, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsllq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllq_count_0]
    vpsllq xmm2, xmm0, xmm1
    SAVE_VPSLLQ 128, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllq_input_a]
    vpsllq xmm2, xmm0, xmmword ptr [rip + vpsllq_count_62]
    SAVE_VPSLLQ 160, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllq_count_63]
    vpsllq xmm0, xmm0, xmm1
    SAVE_VPSLLQ 192, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsllq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllq_count_255]
    vpsllq xmm1, xmm0, xmm1
    SAVE_VPSLLQ 224, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsllq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllq_count_0]
    vpsllq ymm2, ymm0, xmm1
    SAVE_VPSLLQ 256, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsllq_input_a]
    vpsllq ymm2, ymm0, xmmword ptr [rip + vpsllq_count_62]
    SAVE_VPSLLQ 288, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsllq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllq_count_63]
    vpsllq ymm0, ymm0, xmm1
    SAVE_VPSLLQ 320, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsllq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllq_count_255]
    vpsllq ymm1, ymm0, xmm1
    SAVE_VPSLLQ 352, 1
    ret

    .section .data
    .balign 32
vpsllq_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsllq_count_0: .quad 0, 0, 0, 0
vpsllq_count_62: .quad 62, 62, 62, 62
vpsllq_count_63: .quad 63, 63, 63, 63
vpsllq_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsllq_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
