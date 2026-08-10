/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_RESULT
    vmovdqu ymmword ptr [rdi], ymm2
    add rdi, 32
    .endm
    .globl latx_avx_single_vpandn_run
latx_avx_single_vpandn_run:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpandn xmm2, xmm0, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpandn xmm0, xmm0, xmm1
    vmovdqu xmm2, xmm0
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpandn xmm1, xmm0, xmm1
    vmovdqu xmm2, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpandn xmm0, xmm0, xmm0
    vmovdqu xmm2, xmm0
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpandn xmm2, xmm0, xmmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpandn xmm0, xmm0, xmmword ptr [rip + input_b_unaligned]
    vmovdqu xmm2, xmm0
    SAVE_RESULT

    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpandn ymm2, ymm0, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpandn ymm0, ymm0, ymm1
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpandn ymm1, ymm0, ymm1
    vmovdqu ymm2, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpandn ymm0, ymm0, ymm0
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpandn ymm2, ymm0, ymmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpandn ymm0, ymm0, ymmword ptr [rip + input_b_unaligned]
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vzeroupper
    ret

    .globl latx_avx_single_vpandn_fault_xmm
latx_avx_single_vpandn_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpandn xmm2, xmm0, xmmword ptr [rdi]
    ret
    .globl latx_avx_single_vpandn_fault_ymm
latx_avx_single_vpandn_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpandn ymm2, ymm0, ymmword ptr [rdi]
    vzeroupper
    ret
    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .rodata
    .balign 32
input_a:
    .byte 0x00,0xff,0x0f,0xf0,0x55,0xaa,0x33,0xcc,0x80,0x7f,0x01,0xfe,0x12,0x34,0x56,0x78
    .byte 0x11,0xee,0x22,0xdd,0x44,0xbb,0x88,0x77,0x99,0x66,0xab,0xcd,0xde,0xad,0xbe,0xef
input_b_unaligned:
    .byte 0x5a
input_b:
    .byte 0xff,0x00,0xf0,0x0f,0xaa,0x55,0xcc,0x33,0x7f,0x80,0xfe,0x01,0x34,0x12,0x78,0x56
    .byte 0xee,0x11,0xdd,0x22,0xbb,0x44,0x77,0x88,0x66,0x99,0xcd,0xab,0xad,0xde,0xef,0xbe
    .section .note.GNU-stack,"",@progbits
