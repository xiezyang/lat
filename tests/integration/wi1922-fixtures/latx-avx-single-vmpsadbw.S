/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro SAVE_RESULT offset
    vmovdqu ymmword ptr [rdi + \offset], ymm2
    .endm

    .globl latx_avx_single_vmpsadbw_run
    .type latx_avx_single_vmpsadbw_run, @function
latx_avx_single_vmpsadbw_run:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm2, xmm0, xmm1, 0
    SAVE_RESULT 0
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm0, xmm0, xmm1, 1
    vmovdqu xmm2, xmm0
    SAVE_RESULT 32
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm1, xmm0, xmm1, 2
    vmovdqu xmm2, xmm1
    SAVE_RESULT 64
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm0, xmm0, xmm0, 3
    vmovdqu xmm2, xmm0
    SAVE_RESULT 96
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm2, xmm0, xmm1, 4
    SAVE_RESULT 128
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm0, xmm0, xmm1, 5
    vmovdqu xmm2, xmm0
    SAVE_RESULT 160
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm1, xmm0, xmm1, 6
    vmovdqu xmm2, xmm1
    SAVE_RESULT 192
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm0, xmm0, xmm0, 7
    vmovdqu xmm2, xmm0
    SAVE_RESULT 224
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm2, xmm0, xmmword ptr [rip + input_b_unaligned], 8
    SAVE_RESULT 256
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vmpsadbw xmm2, xmm0, xmmword ptr [rip + input_b_unaligned], 255
    SAVE_RESULT 288
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm2, ymm0, ymm1, 0
    SAVE_RESULT 320
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm0, ymm0, ymm1, 1
    vmovdqu ymm2, ymm0
    SAVE_RESULT 352
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm1, ymm0, ymm1, 2
    vmovdqu ymm2, ymm1
    SAVE_RESULT 384
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm0, ymm0, ymm0, 3
    vmovdqu ymm2, ymm0
    SAVE_RESULT 416
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm2, ymm0, ymm1, 4
    SAVE_RESULT 448
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm0, ymm0, ymm1, 5
    vmovdqu ymm2, ymm0
    SAVE_RESULT 480
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm1, ymm0, ymm1, 6
    vmovdqu ymm2, ymm1
    SAVE_RESULT 512
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm0, ymm0, ymm0, 7
    vmovdqu ymm2, ymm0
    SAVE_RESULT 544
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm2, ymm0, ymmword ptr [rip + input_b_unaligned], 8
    SAVE_RESULT 576
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vmpsadbw ymm2, ymm0, ymmword ptr [rip + input_b_unaligned], 255
    SAVE_RESULT 608
    vzeroupper
    ret

    .globl latx_avx_single_vmpsadbw_fault_xmm
    .type latx_avx_single_vmpsadbw_fault_xmm, @function
latx_avx_single_vmpsadbw_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmpsadbw xmm2, xmm0, xmmword ptr [rdi], 7
    ret

    .globl latx_avx_single_vmpsadbw_fault_ymm
    .type latx_avx_single_vmpsadbw_fault_ymm, @function
latx_avx_single_vmpsadbw_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmpsadbw ymm2, ymm0, ymmword ptr [rdi], 7
    vzeroupper
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .rodata
    .balign 32
input_a:
    .byte 0x00, 0xff, 0x10, 0xef, 0x20, 0xdf, 0x30, 0xcf, 0x40, 0xbf, 0x50, 0xaf, 0x60, 0x9f, 0x70, 0x8f, 0x01, 0xfe, 0x11, 0xee, 0x21, 0xde, 0x31, 0xce, 0x41, 0xbe, 0x51, 0xae, 0x61, 0x9e, 0x71, 0x8e
input_b_unaligned:
    .byte 0x5a
    .byte 0xff, 0x00, 0xf0, 0x0f, 0xe0, 0x1f, 0xd0, 0x2f, 0xc0, 0x3f, 0xb0, 0x4f, 0xa0, 0x5f, 0x90, 0x6f, 0xfe, 0x01, 0xee, 0x11, 0xde, 0x21, 0xce, 0x31, 0xbe, 0x41, 0xae, 0x51, 0x9e, 0x61, 0x8e, 0x71
input_b:
    .byte 0xff, 0x00, 0xf0, 0x0f, 0xe0, 0x1f, 0xd0, 0x2f, 0xc0, 0x3f, 0xb0, 0x4f, 0xa0, 0x5f, 0x90, 0x6f, 0xfe, 0x01, 0xee, 0x11, 0xde, 0x21, 0xce, 0x31, 0xbe, 0x41, 0xae, 0x51, 0x9e, 0x61, 0x8e, 0x71

    .section .note.GNU-stack,"",@progbits
