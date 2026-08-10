/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSLLD offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpslld_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpslld_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpslld_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpslld_run
    .type latx_avx_single_vpslld_run, @function
latx_avx_single_vpslld_run:
    vmovdqu xmm0, xmmword ptr [rip + vpslld_input_a]
    vpslld xmm2, xmm0, 0
    SAVE_VPSLLD 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpslld_input_a]
    vpslld xmm0, xmm0, 30
    SAVE_VPSLLD 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpslld_input_a]
    vpslld xmm2, xmm0, 31
    SAVE_VPSLLD 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpslld_input_a]
    vpslld xmm0, xmm0, 255
    SAVE_VPSLLD 96, 0
    vmovdqu xmm0, xmmword ptr [rip + vpslld_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpslld_count_0]
    vpslld xmm2, xmm0, xmm1
    SAVE_VPSLLD 128, 2
    vmovdqu xmm0, xmmword ptr [rip + vpslld_input_a]
    vpslld xmm2, xmm0, xmmword ptr [rip + vpslld_count_30]
    SAVE_VPSLLD 160, 2
    vmovdqu xmm0, xmmword ptr [rip + vpslld_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpslld_count_31]
    vpslld xmm0, xmm0, xmm1
    SAVE_VPSLLD 192, 0
    vmovdqu xmm0, xmmword ptr [rip + vpslld_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpslld_count_255]
    vpslld xmm1, xmm0, xmm1
    SAVE_VPSLLD 224, 1
    vmovdqu ymm0, ymmword ptr [rip + vpslld_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpslld_count_0]
    vpslld ymm2, ymm0, xmm1
    SAVE_VPSLLD 256, 2
    vmovdqu ymm0, ymmword ptr [rip + vpslld_input_a]
    vpslld ymm2, ymm0, xmmword ptr [rip + vpslld_count_30]
    SAVE_VPSLLD 288, 2
    vmovdqu ymm0, ymmword ptr [rip + vpslld_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpslld_count_31]
    vpslld ymm0, ymm0, xmm1
    SAVE_VPSLLD 320, 0
    vmovdqu ymm0, ymmword ptr [rip + vpslld_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpslld_count_255]
    vpslld ymm1, ymm0, xmm1
    SAVE_VPSLLD 352, 1
    ret

    .section .data
    .balign 32
vpslld_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpslld_count_0: .quad 0, 0, 0, 0
vpslld_count_30: .quad 30, 30, 30, 30
vpslld_count_31: .quad 31, 31, 31, 31
vpslld_count_255: .quad 255, 255, 255, 255
    .balign 64
vpslld_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
