/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSLLDQ offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpslldq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpslldq_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpslldq_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpslldq_run
    .type latx_avx_single_vpslldq_run, @function
latx_avx_single_vpslldq_run:
    vmovdqu xmm0, xmmword ptr [rip + vpslldq_input_a]
    vpslldq xmm2, xmm0, 0
    SAVE_VPSLLDQ 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpslldq_input_a]
    vpslldq xmm0, xmm0, 1
    SAVE_VPSLLDQ 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpslldq_input_a]
    vpslldq xmm2, xmm0, 15
    SAVE_VPSLLDQ 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpslldq_input_a]
    vpslldq xmm0, xmm0, 16
    SAVE_VPSLLDQ 96, 0
    vmovdqu ymm0, ymmword ptr [rip + vpslldq_input_a]
    vpslldq ymm2, ymm0, 255
    SAVE_VPSLLDQ 128, 2
    vmovdqu ymm0, ymmword ptr [rip + vpslldq_input_a]
    vpslldq ymm0, ymm0, 0
    SAVE_VPSLLDQ 160, 0
    vmovdqu ymm0, ymmword ptr [rip + vpslldq_input_a]
    vpslldq ymm2, ymm0, 15
    SAVE_VPSLLDQ 192, 2
    vmovdqu ymm0, ymmword ptr [rip + vpslldq_input_a]
    vpslldq ymm0, ymm0, 16
    SAVE_VPSLLDQ 224, 0
    ret

    .section .data
    .balign 32
vpslldq_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210

    .balign 64
vpslldq_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
