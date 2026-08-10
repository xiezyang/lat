/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_VPMOVMSKB offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpmovmskb_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpmovmskb_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vpmovmskb_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vpmovmskb_run
    .type latx_avx_single_vpmovmskb_run, @function
latx_avx_single_vpmovmskb_run:
    test rsi, rsi
    jnz .Lunsupported
    vmovdqu xmm0, xmmword ptr [rip + vpmovmskb_input_a]
    vpmovmskb eax, xmm0
    mov qword ptr [rdi + 0], rax
    vmovdqu xmm1, xmmword ptr [rip + vpmovmskb_input_b]
    vpmovmskb eax, xmm1
    mov qword ptr [rdi + 8], rax
    vmovdqu xmm0, xmmword ptr [rip + vpmovmskb_input_a]
    vpmovmskb eax, xmm0
    mov qword ptr [rdi + 16], rax
    vmovdqu xmm1, xmmword ptr [rip + vpmovmskb_input_b]
    vpmovmskb eax, xmm1
    mov qword ptr [rdi + 24], rax
    vmovdqu ymm0, ymmword ptr [rip + vpmovmskb_input_a]
    vpmovmskb eax, ymm0
    mov qword ptr [rdi + 32], rax
    vmovdqu ymm1, ymmword ptr [rip + vpmovmskb_input_b]
    vpmovmskb eax, ymm1
    mov qword ptr [rdi + 40], rax
    vmovdqu ymm0, ymmword ptr [rip + vpmovmskb_input_a]
    vpmovmskb eax, ymm0
    mov qword ptr [rdi + 48], rax
    vmovdqu ymm1, ymmword ptr [rip + vpmovmskb_input_b]
    vpmovmskb eax, ymm1
    mov qword ptr [rdi + 56], rax
    ret
.Lunsupported:
    mov eax, 2
    ret

    .section .data
    .balign 32
vpmovmskb_input_a: .byte 0x80,0x7f,0x01,0xff,0x00,0x7e,0x02,0xfe,0x11,0xee,0x22,0xdd,0x33,0xcc,0x44,0xbb,0x55,0xaa,0x66,0x99,0x77,0x88,0x10,0xf0,0x20,0xe0,0x30,0xd0,0x40,0xc0,0x50,0xb0
vpmovmskb_input_b: .byte 0x7f,0x80,0xff,0x01,0x7e,0x82,0xfe,0x02,0xee,0x11,0xdd,0x22,0xcc,0x33,0xbb,0x44,0xaa,0x55,0x99,0x66,0x88,0x77,0xf0,0x10,0xe0,0x20,0xd0,0x30,0xc0,0x40,0xb0,0x50
    .balign 64
vpmovmskb_xsave_area: .zero 4096

    .section .note.GNU-stack,"",@progbits
