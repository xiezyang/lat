/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_VPSIGND offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpsignd_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpsignd_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpsignd_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpsignd_run
    .type latx_avx_single_vpsignd_run, @function
latx_avx_single_vpsignd_run:
    test rsi, rsi
    jnz .Lfault
    vmovdqu xmm0, xmmword ptr [rip + vpsignd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsignd_input_b]
    vpsignd xmm2, xmm0, xmm1
    SAVE_VPSIGND 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsignd_input_a]
    vmovdqu xmm1, xmmword ptr [rip + vpsignd_input_b]
    vpsignd xmm0, xmm0, xmm1
    SAVE_VPSIGND 32, 0
    vmovdqu xmm0, xmmword ptr [rip + vpsignd_input_a]
    vpsignd xmm2, xmm0, xmmword ptr [rip + vpsignd_input_b]
    SAVE_VPSIGND 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpsignd_input_a]
    vpsignd xmm0, xmm0, xmmword ptr [rip + vpsignd_input_b]
    SAVE_VPSIGND 96, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsignd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsignd_input_b]
    vpsignd ymm2, ymm0, ymm1
    SAVE_VPSIGND 128, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsignd_input_a]
    vmovdqu ymm1, ymmword ptr [rip + vpsignd_input_b]
    vpsignd ymm0, ymm0, ymm1
    SAVE_VPSIGND 160, 0
    vmovdqu ymm0, ymmword ptr [rip + vpsignd_input_a]
    vpsignd ymm2, ymm0, ymmword ptr [rip + vpsignd_input_b]
    SAVE_VPSIGND 192, 2
    vmovdqu ymm0, ymmword ptr [rip + vpsignd_input_a]
    vpsignd ymm0, ymm0, ymmword ptr [rip + vpsignd_input_b]
    SAVE_VPSIGND 224, 0
    ret
.Lfault:
    vpsignd ymm2, ymm0, ymmword ptr [rsi]
    ret

    .section .data
    .balign 32
vpsignd_input_a: .byte 0x80,0x7f,0x01,0xff,0x00,0x7e,0x02,0xfe,0x11,0xee,0x22,0xdd,0x33,0xcc,0x44,0xbb,0x55,0xaa,0x66,0x99,0x77,0x88,0x10,0xf0,0x20,0xe0,0x30,0xd0,0x40,0xc0,0x50,0xb0
vpsignd_input_b: .byte 0x7f,0x80,0xff,0x01,0x7e,0x82,0xfe,0x02,0xee,0x11,0xdd,0x22,0xcc,0x33,0xbb,0x44,0xaa,0x55,0x99,0x66,0x88,0x77,0xf0,0x10,0xe0,0x20,0xd0,0x30,0xc0,0x40,0xb0,0x50
    .balign 64
vpsignd_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
