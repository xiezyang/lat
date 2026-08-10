/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_VPMULHRSW offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpmulhrsw_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpmulhrsw_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpmulhrsw_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpmulhrsw_run
    .type latx_avx_single_vpmulhrsw_run, @function
latx_avx_single_vpmulhrsw_run:
    test rsi, rsi
    jnz .Lfault
    vmovdqu xmm0, xmmword ptr [rip + vpmulhrsw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpmulhrsw_input_b]
    vpmulhrsw xmm2, xmm0, xmm1
    SAVE_VPMULHRSW 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpmulhrsw_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpmulhrsw_input_b]
    vpmulhrsw xmm0, xmm0, xmm1
    SAVE_VPMULHRSW 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpmulhrsw_input_a]
    vpmulhrsw xmm2, xmm0, xmmword ptr [rip + vpmulhrsw_input_b]
    SAVE_VPMULHRSW 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpmulhrsw_input_a]
    vpmulhrsw xmm0, xmm0, xmmword ptr [rip + vpmulhrsw_input_b]
    SAVE_VPMULHRSW 96, 0
    vmovdqu ymm0, ymmword ptr [rip + vpmulhrsw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpmulhrsw_input_b]
    vpmulhrsw ymm2, ymm0, ymm1
    SAVE_VPMULHRSW 128, 2
    vmovdqu ymm0, ymmword ptr [rip + vpmulhrsw_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpmulhrsw_input_b]
    vpmulhrsw ymm0, ymm0, ymm1
    SAVE_VPMULHRSW 160, 0
    vmovdqu ymm0, ymmword ptr [rip + vpmulhrsw_input_a]
    vpmulhrsw ymm2, ymm0, ymmword ptr [rip + vpmulhrsw_input_b]
    SAVE_VPMULHRSW 192, 2
    vmovdqu ymm0, ymmword ptr [rip + vpmulhrsw_input_a]
    vpmulhrsw ymm0, ymm0, ymmword ptr [rip + vpmulhrsw_input_b]
    SAVE_VPMULHRSW 224, 0
    ret
.Lfault:
    vpmulhrsw ymm2, ymm0, ymmword ptr [rsi]
    ret

    .section .data
    .balign 32
vpmulhrsw_input_a: .byte 0x80,0x7f,0x01,0xff,0x00,0x7e,0x02,0xfe,0x11,0xee,0x22,0xdd,0x33,0xcc,0x44,0xbb,0x55,0xaa,0x66,0x99,0x77,0x88,0x10,0xf0,0x20,0xe0,0x30,0xd0,0x40,0xc0,0x50,0xb0
vpmulhrsw_input_b: .byte 0x7f,0x80,0xff,0x01,0x7e,0x82,0xfe,0x02,0xee,0x11,0xdd,0x22,0xcc,0x33,0xbb,0x44,0xaa,0x55,0x99,0x66,0x88,0x77,0xf0,0x10,0xe0,0x20,0xd0,0x30,0xc0,0x40,0xb0,0x50
    .balign 64
vpmulhrsw_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
