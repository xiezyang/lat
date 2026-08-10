/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef LATX_VINTCMP_MNEMONIC
#error "LATX_VINTCMP_MNEMONIC must be defined by the fixture wrapper"
#endif
#ifndef LATX_VINTCMP_SYMBOL_PREFIX
#error "LATX_VINTCMP_SYMBOL_PREFIX must be defined by the fixture wrapper"
#endif

#define LATX_VINTCMP_JOIN_INNER(a, b) a##b
#define LATX_VINTCMP_JOIN(a, b) LATX_VINTCMP_JOIN_INNER(a, b)
#define LATX_VINTCMP_SYMBOL(suffix) \
    LATX_VINTCMP_JOIN(LATX_VINTCMP_SYMBOL_PREFIX, suffix)

    .intel_syntax noprefix
    .text

    .macro LATX_VINTCMP_RESTORE
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + LATX_VINTCMP_SYMBOL(_xrstor_area)]
    movdqu xmm0, xmmword ptr [rip + LATX_VINTCMP_SYMBOL(_seed0)]
    movdqu xmm1, xmmword ptr [rip + LATX_VINTCMP_SYMBOL(_seed1)]
    movdqu xmm2, xmmword ptr [rip + LATX_VINTCMP_SYMBOL(_seed2)]
    .endm

    .macro LATX_VINTCMP_SAVE_RESULT index
    test rdi, rdi
    jz .Llatx_vintcmp_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + LATX_VINTCMP_SYMBOL(_xsave_area) + 160], xmm14
    movdqu xmmword ptr [rip + LATX_VINTCMP_SYMBOL(_xsave_area) + 576], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + LATX_VINTCMP_SYMBOL(_xsave_area)]
    movdqu xmm14, xmmword ptr [rip + LATX_VINTCMP_SYMBOL(_xsave_area) + 160]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + LATX_VINTCMP_SYMBOL(_xsave_area) + 576]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Llatx_vintcmp_skip_save_\@:
    .endm

    .macro LATX_VINTCMP_REG_CASE width, variant, index
    LATX_VINTCMP_RESTORE
    .if \width == 128
        .if \variant == 0
        LATX_VINTCMP_MNEMONIC xmm0, xmm1, xmm2
        .elseif \variant == 1
        LATX_VINTCMP_MNEMONIC xmm0, xmm0, xmm2
        .else
        LATX_VINTCMP_MNEMONIC xmm0, xmm1, xmm0
        .endif
    .else
        .if \variant == 0
        LATX_VINTCMP_MNEMONIC ymm0, ymm1, ymm2
        .elseif \variant == 1
        LATX_VINTCMP_MNEMONIC ymm0, ymm0, ymm2
        .else
        LATX_VINTCMP_MNEMONIC ymm0, ymm1, ymm0
        .endif
    .endif
    LATX_VINTCMP_SAVE_RESULT \index
    .endm

    .macro LATX_VINTCMP_MEM_CASE width, variant, index
    LATX_VINTCMP_RESTORE
    .if \width == 128
        .if \variant == 0
        LATX_VINTCMP_MNEMONIC xmm0, xmm1, xmmword ptr [rsi]
        .else
        LATX_VINTCMP_MNEMONIC xmm0, xmm0, xmmword ptr [rsi + 16]
        .endif
    .else
        .if \variant == 0
        LATX_VINTCMP_MNEMONIC ymm0, ymm1, ymmword ptr [rsi]
        .else
        LATX_VINTCMP_MNEMONIC ymm0, ymm0, ymmword ptr [rsi + 32]
        .endif
    .endif
    LATX_VINTCMP_SAVE_RESULT \index
    .endm

    .globl LATX_VINTCMP_SYMBOL(_run)
    .type LATX_VINTCMP_SYMBOL(_run), @function
LATX_VINTCMP_SYMBOL(_run):
    LATX_VINTCMP_REG_CASE 128, 0, 0
    LATX_VINTCMP_REG_CASE 128, 1, 1
    LATX_VINTCMP_REG_CASE 128, 2, 2
    LATX_VINTCMP_MEM_CASE 128, 0, 3
    LATX_VINTCMP_MEM_CASE 128, 1, 4
    LATX_VINTCMP_REG_CASE 256, 0, 5
    LATX_VINTCMP_REG_CASE 256, 1, 6
    LATX_VINTCMP_REG_CASE 256, 2, 7
    LATX_VINTCMP_MEM_CASE 256, 0, 8
    LATX_VINTCMP_MEM_CASE 256, 1, 9
    ret

    .section .rodata
    .balign 32
LATX_VINTCMP_SYMBOL(_seed0):
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
LATX_VINTCMP_SYMBOL(_seed1):
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f851
LATX_VINTCMP_SYMBOL(_seed2):
    .quad 0x1122334455667788, 0x99aabbccddeeff01

    .section .data
    .balign 64
LATX_VINTCMP_SYMBOL(_xrstor_area):
    .short 0x037f
    .short 0
    .zero 20
    .long 0x00001f80
    .long 0x0000ffff
    .zero 128
    .zero 256
    .zero 96
    .quad 6
    .quad 0
    .zero 48
    .quad 0x0102030405060708, 0x1112131415161718
    .quad 0x8182838485868788, 0x9192939495969798
    .quad 0xa1a2a3a4a5a6a7a8, 0xb1b2b3b4b5b6b7b8
    .zero 192
    .quad 0xf1f2f3f4f5f6f7f8, 0x0101010102020202

    .section .bss
    .balign 64
LATX_VINTCMP_SYMBOL(_xsave_area):
    .zero 4096

    .section .note.GNU-stack,"",@progbits
