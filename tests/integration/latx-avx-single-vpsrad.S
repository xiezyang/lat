/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .macro SAVE_VPSRAD offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsrad_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsrad_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsrad_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsrad_run
    .type latx_avx_single_vpsrad_run, @function
latx_avx_single_vpsrad_run:
    vmovdqu xmm0, xmmword ptr [rip + vpsrad_input_a]
    vpsrad xmm2, xmm0, 0
    SAVE_VPSRAD 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrad_input_a]
    vpsrad xmm0, xmm0, 30
    SAVE_VPSRAD 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrad_input_a]
    vpsrad xmm2, xmm0, 31
    SAVE_VPSRAD 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrad_input_a]
    vpsrad xmm0, xmm0, 255
    SAVE_VPSRAD 96, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrad_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrad_count_0]
    vpsrad xmm2, xmm0, xmm1
    SAVE_VPSRAD 128, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrad_input_a]
    vpsrad xmm2, xmm0, xmmword ptr [rip + vpsrad_count_30]
    SAVE_VPSRAD 160, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsrad_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrad_count_31]
    vpsrad xmm0, xmm0, xmm1
    SAVE_VPSRAD 192, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsrad_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsrad_count_255]
    vpsrad xmm1, xmm0, xmm1
    SAVE_VPSRAD 224, 1
    vmovdqu ymm0, ymmword ptr [rip + vpsrad_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrad_count_0]
    vpsrad ymm2, ymm0, xmm1
    SAVE_VPSRAD 256, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrad_input_a]
    vpsrad ymm2, ymm0, xmmword ptr [rip + vpsrad_count_30]
    SAVE_VPSRAD 288, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsrad_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrad_count_31]
    vpsrad ymm0, ymm0, xmm1
    SAVE_VPSRAD 320, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsrad_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsrad_count_255]
    vpsrad ymm1, ymm0, xmm1
    SAVE_VPSRAD 352, 1
    ret

    .section .data
    .balign 32
vpsrad_input_a: .quad 0x80017fff00010000, 0x7fff80010000ffff, 0x0123456789abcdef, 0xfedcba9876543210
vpsrad_count_0: .quad 0, 0, 0, 0
vpsrad_count_30: .quad 30, 30, 30, 30
vpsrad_count_31: .quad 31, 31, 31, 31
vpsrad_count_255: .quad 255, 255, 255, 255
    .balign 64
vpsrad_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
