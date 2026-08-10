/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_VPMOVSXDQ offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpmovsxdq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpmovsxdq_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpmovsxdq_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpmovsxdq_run
    .type latx_avx_single_vpmovsxdq_run, @function
latx_avx_single_vpmovsxdq_run:
    test rsi, rsi
    jnz .Lfault
    vmovdqu xmm0, xmmword ptr [rip + vpmovsxdq_input_a]
    vpmovsxdq xmm2, xmm0
    SAVE_VPMOVSXDQ 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpmovsxdq_input_a]
    vpmovsxdq xmm0, xmm0
    SAVE_VPMOVSXDQ 32, 0
    vpmovsxdq xmm2, qword ptr [rip + vpmovsxdq_input_a]
    SAVE_VPMOVSXDQ 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpmovsxdq_input_a]
    vpmovsxdq ymm2, xmm0
    SAVE_VPMOVSXDQ 96, 2
    vmovdqu xmm0, xmmword ptr [rip + vpmovsxdq_input_a]
    vpmovsxdq ymm0, xmm0
    SAVE_VPMOVSXDQ 128, 0
    vpmovsxdq ymm2, xmmword ptr [rip + vpmovsxdq_input_a]
    SAVE_VPMOVSXDQ 160, 2
    vpmovsxdq xmm0, qword ptr [rip + vpmovsxdq_input_a]
    SAVE_VPMOVSXDQ 192, 0
    vpmovsxdq ymm0, xmmword ptr [rip + vpmovsxdq_input_a]
    SAVE_VPMOVSXDQ 224, 0
    ret
.Lfault:
    vpmovsxdq ymm2, xmmword ptr [rsi]
    ret

    .section .data
    .balign 32
vpmovsxdq_input_a: .byte 0x80,0x7f,0x01,0xff,0x00,0x7e,0x02,0xfe,0x11,0xee,0x22,0xdd,0x33,0xcc,0x44,0xbb,0x55,0xaa,0x66,0x99,0x77,0x88,0x10,0xf0,0x20,0xe0,0x30,0xd0,0x40,0xc0,0x50,0xb0
vpmovsxdq_input_b: .byte 0x7f,0x80,0xff,0x01,0x7e,0x82,0xfe,0x02,0xee,0x11,0xdd,0x22,0xcc,0x33,0xbb,0x44,0xaa,0x55,0x99,0x66,0x88,0x77,0xf0,0x10,0xe0,0x20,0xd0,0x30,0xc0,0x40,0xb0,0x50
    .balign 64
vpmovsxdq_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
