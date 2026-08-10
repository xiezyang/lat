/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSLLVQ offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsllvq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsllvq_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsllvq_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsllvq_run
    .type latx_avx_single_vpsllvq_run, @function
latx_avx_single_vpsllvq_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsllvq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllvq_count_0]
    vpsllvq xmm2, xmm0, xmm1
    SAVE_VPSLLVQ 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllvq_input_a]
    vpsllvq xmm2, xmm0, xmmword ptr [rip + vpsllvq_count_62]
    SAVE_VPSLLVQ 32, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllvq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllvq_count_63]
    vpsllvq xmm0, xmm0, xmm1
    SAVE_VPSLLVQ 64, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsllvq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllvq_count_255]
    vpsllvq xmm1, xmm0, xmm1
    SAVE_VPSLLVQ 96, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsllvq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllvq_count_0]
    vpsllvq ymm2, ymm0, ymm1
    SAVE_VPSLLVQ 128, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsllvq_input_a]
    vpsllvq ymm2, ymm0, ymmword ptr [rip + vpsllvq_count_62]
    SAVE_VPSLLVQ 160, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsllvq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllvq_count_63]
    vpsllvq ymm0, ymm0, ymm1
    SAVE_VPSLLVQ 192, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsllvq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllvq_count_255]
    vpsllvq ymm1, ymm0, ymm1
    SAVE_VPSLLVQ 224, 1
    ret

    .section .data
    .balign 32
vpsllvq_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsllvq_count_0: .quad 0, 0, 0, 0
vpsllvq_count_62: .quad 62, 62, 62, 62
vpsllvq_count_63: .quad 63, 63, 63, 63
vpsllvq_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsllvq_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
