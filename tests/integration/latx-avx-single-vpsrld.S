/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSRLD offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsrld_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsrld_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsrld_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsrld_run
    .type latx_avx_single_vpsrld_run, @function
latx_avx_single_vpsrld_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsrld_input_a]
    vpsrld xmm2, xmm0, 0
    SAVE_VPSRLD 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrld_input_a]
    vpsrld xmm0, xmm0, 30
    SAVE_VPSRLD 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrld_input_a]
    vpsrld xmm2, xmm0, 31
    SAVE_VPSRLD 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrld_input_a]
    vpsrld xmm0, xmm0, 255
    SAVE_VPSRLD 96, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrld_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrld_count_0]
    vpsrld xmm2, xmm0, xmm1
    SAVE_VPSRLD 128, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrld_input_a]
    vpsrld xmm2, xmm0, xmmword ptr [rip + vpsrld_count_30]
    SAVE_VPSRLD 160, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrld_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrld_count_31]
    vpsrld xmm0, xmm0, xmm1
    SAVE_VPSRLD 192, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrld_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrld_count_255]
    vpsrld xmm1, xmm0, xmm1
    SAVE_VPSRLD 224, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsrld_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrld_count_0]
    vpsrld ymm2, ymm0, xmm1
    SAVE_VPSRLD 256, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrld_input_a]
    vpsrld ymm2, ymm0, xmmword ptr [rip + vpsrld_count_30]
    SAVE_VPSRLD 288, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrld_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrld_count_31]
    vpsrld ymm0, ymm0, xmm1
    SAVE_VPSRLD 320, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsrld_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrld_count_255]
    vpsrld ymm1, ymm0, xmm1
    SAVE_VPSRLD 352, 1
    ret

    .section .data
    .balign 32
vpsrld_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsrld_count_0: .quad 0, 0, 0, 0
vpsrld_count_30: .quad 30, 30, 30, 30
vpsrld_count_31: .quad 31, 31, 31, 31
vpsrld_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsrld_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
