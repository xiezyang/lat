/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_VPMOVSXBD offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpmovsxbd_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpmovsxbd_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpmovsxbd_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpmovsxbd_run
    .type latx_avx_single_vpmovsxbd_run, @function
latx_avx_single_vpmovsxbd_run:
    test rsi, rsi
    jnz .Lfault
    vmovdqu xmm0, xmmword ptr [rip + vpmovsxbd_input_a]
    vpmovsxbd xmm2, xmm0
    SAVE_VPMOVSXBD 0, 2
    vmovdqu xmm0, xmmword ptr [rip + vpmovsxbd_input_a]
    vpmovsxbd xmm0, xmm0
    SAVE_VPMOVSXBD 32, 0
    vpmovsxbd xmm2, dword ptr [rip + vpmovsxbd_input_a]
    SAVE_VPMOVSXBD 64, 2
    vmovdqu xmm0, xmmword ptr [rip + vpmovsxbd_input_a]
    vpmovsxbd ymm2, xmm0
    SAVE_VPMOVSXBD 96, 2
    vmovdqu xmm0, xmmword ptr [rip + vpmovsxbd_input_a]
    vpmovsxbd ymm0, xmm0
    SAVE_VPMOVSXBD 128, 0
    vpmovsxbd ymm2, qword ptr [rip + vpmovsxbd_input_a]
    SAVE_VPMOVSXBD 160, 2
    vpmovsxbd xmm0, dword ptr [rip + vpmovsxbd_input_a]
    SAVE_VPMOVSXBD 192, 0
    vpmovsxbd ymm0, qword ptr [rip + vpmovsxbd_input_a]
    SAVE_VPMOVSXBD 224, 0
    ret
.Lfault:
    vpmovsxbd ymm2, qword ptr [rsi]
    ret

    .section .data
    .balign 32
vpmovsxbd_input_a: .byte 0x80,0x7f,0x01,0xff,0x00,0x7e,0x02,0xfe,0x11,0xee,0x22,0xdd,0x33,0xcc,0x44,0xbb,0x55,0xaa,0x66,0x99,0x77,0x88,0x10,0xf0,0x20,0xe0,0x30,0xd0,0x40,0xc0,0x50,0xb0
vpmovsxbd_input_b: .byte 0x7f,0x80,0xff,0x01,0x7e,0x82,0xfe,0x02,0xee,0x11,0xdd,0x22,0xcc,0x33,0xbb,0x44,0xaa,0x55,0x99,0x66,0x88,0x77,0xf0,0x10,0xe0,0x20,0xd0,0x30,0xc0,0x40,0xb0,0x50
    .balign 64
vpmovsxbd_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
