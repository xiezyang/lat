/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .globl latx_avx_single_vpshufhw_run
    .type latx_avx_single_vpshufhw_run, @function
latx_avx_single_vpshufhw_run:
    movdqu xmm0, xmmword ptr [rip + data_a]
    vpshufhw xmm2, xmm0, 0x00
    vmovdqu ymmword ptr [rdi+0], ymm2
    movdqu xmm0, xmmword ptr [rip + data_a]
    vpshufhw xmm0, xmm0, 0x1b
    vmovdqu ymmword ptr [rdi+32], ymm0
    movdqu xmm0, xmmword ptr [rip + data_a]
    vpshufhw xmm1, xmm0, 0xff
    vmovdqu ymmword ptr [rdi+64], ymm1
    vpshufhw xmm2, xmmword ptr [rip + data_b], 0xff
    vmovdqu ymmword ptr [rdi+96], ymm2
    vmovdqu ymm0, ymmword ptr [rip + data_a]
    vpshufhw ymm2, ymm0, 0x00
    vmovdqu ymmword ptr [rdi+128], ymm2
    vmovdqu ymm0, ymmword ptr [rip + data_a]
    vpshufhw ymm0, ymm0, 0x1b
    vmovdqu ymmword ptr [rdi+160], ymm0
    vmovdqu ymm0, ymmword ptr [rip + data_a]
    vpshufhw ymm1, ymm0, 0xff
    vmovdqu ymmword ptr [rdi+192], ymm1
    vpshufhw ymm2, ymmword ptr [rip + data_b], 0xff
    vmovdqu ymmword ptr [rdi+224], ymm2
    vzeroupper
    ret

    .globl latx_avx_single_vpshufhw_fault
    .type latx_avx_single_vpshufhw_fault, @function
latx_avx_single_vpshufhw_fault:
    vmovdqu ymm0, ymmword ptr [rip + data_a]
    vpshufhw ymm1, ymmword ptr [rdi], 0xff
    ret

    .section .rodata
    .balign 32
data_a:
    .byte 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07
    .byte 0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    .byte 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17
    .byte 0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
data_b:
    .byte 0x80,0x01,0x02,0x83,0x04,0x05,0x86,0x07
    .byte 0x08,0x89,0x0a,0x0b,0x8c,0x0d,0x0e,0x0f
    .byte 0x10,0x91,0x12,0x13,0x14,0x95,0x16,0x17
    .byte 0x18,0x19,0x9a,0x1b,0x1c,0x1d,0x1e,0x9f

    .section .note.GNU-stack,"",@progbits
