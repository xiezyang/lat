/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSRLVQ offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsrlvq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsrlvq_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsrlvq_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsrlvq_run
    .type latx_avx_single_vpsrlvq_run, @function
latx_avx_single_vpsrlvq_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsrlvq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlvq_count_0]
    vpsrlvq xmm2, xmm0, xmm1
    SAVE_VPSRLVQ 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlvq_input_a]
    vpsrlvq xmm2, xmm0, xmmword ptr [rip + vpsrlvq_count_62]
    SAVE_VPSRLVQ 32, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlvq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlvq_count_63]
    vpsrlvq xmm0, xmm0, xmm1
    SAVE_VPSRLVQ 64, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrlvq_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlvq_count_255]
    vpsrlvq xmm1, xmm0, xmm1
    SAVE_VPSRLVQ 96, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsrlvq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlvq_count_0]
    vpsrlvq ymm2, ymm0, ymm1
    SAVE_VPSRLVQ 128, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrlvq_input_a]
    vpsrlvq ymm2, ymm0, ymmword ptr [rip + vpsrlvq_count_62]
    SAVE_VPSRLVQ 160, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrlvq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlvq_count_63]
    vpsrlvq ymm0, ymm0, ymm1
    SAVE_VPSRLVQ 192, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsrlvq_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlvq_count_255]
    vpsrlvq ymm1, ymm0, ymm1
    SAVE_VPSRLVQ 224, 1
    ret

    .section .data
    .balign 32
vpsrlvq_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsrlvq_count_0: .quad 0, 0, 0, 0
vpsrlvq_count_62: .quad 62, 62, 62, 62
vpsrlvq_count_63: .quad 63, 63, 63, 63
vpsrlvq_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsrlvq_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
