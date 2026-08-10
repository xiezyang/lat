/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSLLVD offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsllvd_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsllvd_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsllvd_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsllvd_run
    .type latx_avx_single_vpsllvd_run, @function
latx_avx_single_vpsllvd_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsllvd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllvd_count_0]
    vpsllvd xmm2, xmm0, xmm1
    SAVE_VPSLLVD 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllvd_input_a]
    vpsllvd xmm2, xmm0, xmmword ptr [rip + vpsllvd_count_30]
    SAVE_VPSLLVD 32, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsllvd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllvd_count_31]
    vpsllvd xmm0, xmm0, xmm1
    SAVE_VPSLLVD 64, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsllvd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsllvd_count_255]
    vpsllvd xmm1, xmm0, xmm1
    SAVE_VPSLLVD 96, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsllvd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllvd_count_0]
    vpsllvd ymm2, ymm0, ymm1
    SAVE_VPSLLVD 128, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsllvd_input_a]
    vpsllvd ymm2, ymm0, ymmword ptr [rip + vpsllvd_count_30]
    SAVE_VPSLLVD 160, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsllvd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllvd_count_31]
    vpsllvd ymm0, ymm0, ymm1
    SAVE_VPSLLVD 192, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsllvd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsllvd_count_255]
    vpsllvd ymm1, ymm0, ymm1
    SAVE_VPSLLVD 224, 1
    ret

    .section .data
    .balign 32
vpsllvd_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsllvd_count_0: .quad 0, 0, 0, 0
vpsllvd_count_30: .quad 30, 30, 30, 30
vpsllvd_count_31: .quad 31, 31, 31, 31
vpsllvd_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsllvd_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
