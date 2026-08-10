/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSRLVD offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsrlvd_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsrlvd_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsrlvd_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsrlvd_run
    .type latx_avx_single_vpsrlvd_run, @function
latx_avx_single_vpsrlvd_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsrlvd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlvd_count_0]
    vpsrlvd xmm2, xmm0, xmm1
    SAVE_VPSRLVD 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlvd_input_a]
    vpsrlvd xmm2, xmm0, xmmword ptr [rip + vpsrlvd_count_30]
    SAVE_VPSRLVD 32, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrlvd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlvd_count_31]
    vpsrlvd xmm0, xmm0, xmm1
    SAVE_VPSRLVD 64, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrlvd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrlvd_count_255]
    vpsrlvd xmm1, xmm0, xmm1
    SAVE_VPSRLVD 96, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsrlvd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlvd_count_0]
    vpsrlvd ymm2, ymm0, ymm1
    SAVE_VPSRLVD 128, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrlvd_input_a]
    vpsrlvd ymm2, ymm0, ymmword ptr [rip + vpsrlvd_count_30]
    SAVE_VPSRLVD 160, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrlvd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlvd_count_31]
    vpsrlvd ymm0, ymm0, ymm1
    SAVE_VPSRLVD 192, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsrlvd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrlvd_count_255]
    vpsrlvd ymm1, ymm0, ymm1
    SAVE_VPSRLVD 224, 1
    ret

    .section .data
    .balign 32
vpsrlvd_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsrlvd_count_0: .quad 0, 0, 0, 0
vpsrlvd_count_30: .quad 30, 30, 30, 30
vpsrlvd_count_31: .quad 31, 31, 31, 31
vpsrlvd_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsrlvd_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
