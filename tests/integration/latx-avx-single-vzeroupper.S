/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_main
    .type latx_avx_single_main, @function
latx_avx_single_main:
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vzeroupper_xrstor_area]

    /* This is the only AVX instruction in the probe. */
    vzeroupper

    mov eax, 6
    xor edx, edx
    xsave64 [rip + vzeroupper_xsave_area]

    mov eax, 1
    mov edi, 1
    lea rsi, [rip + vzeroupper_xsave_area + 160]
    mov edx, 32
    syscall
    xor eax, eax
    ret

    .section .data
    .balign 64
vzeroupper_xrstor_area:
    .short 0x037f
    .short 0
    .zero 20
    .long 0x00001f80
    .long 0x0000ffff
    .zero 128
    .quad 0x0123456789abcdef, 0xfedcba9876543210
    .zero 336
    .quad 6
    .quad 0
    .zero 48
    .quad 0x1122334455667788, 0x99aabbccddeeff00

    .section .bss
    .balign 64
vzeroupper_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
