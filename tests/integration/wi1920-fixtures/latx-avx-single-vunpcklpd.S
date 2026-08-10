/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro SAVE_RESULT
    vmovdqu ymmword ptr [rdi], ymm2
    add rdi, 32
    .endm

    .globl latx_avx_single_vunpcklpd_run
    .type latx_avx_single_vunpcklpd_run, @function
latx_avx_single_vunpcklpd_run:
    /* XMM register source, destination aliases src1, src2, and both. */
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vunpcklpd xmm2, xmm0, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vunpcklpd xmm0, xmm0, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vunpcklpd xmm1, xmm0, xmm1
    vmovdqu xmm2, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vunpcklpd xmm0, xmm0, xmm0
    vmovdqu xmm2, xmm0
    SAVE_RESULT

    /* XMM memory source, with destination/src1 aliases. */
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vunpcklpd xmm2, xmm0, xmmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vunpcklpd xmm0, xmm0, xmmword ptr [rip + input_b_unaligned]
    vmovdqu xmm2, xmm0
    SAVE_RESULT

    /* YMM register source, covering both independent 128-bit lanes. */
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vunpcklpd ymm2, ymm0, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vunpcklpd ymm0, ymm0, ymm1
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vunpcklpd ymm1, ymm0, ymm1
    vmovdqu ymm2, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vunpcklpd ymm0, ymm0, ymm0
    vmovdqu ymm2, ymm0
    SAVE_RESULT

    /* YMM memory source, including an unaligned legal address. */
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vunpcklpd ymm2, ymm0, ymmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vunpcklpd ymm0, ymm0, ymmword ptr [rip + input_b_unaligned]
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vzeroupper
    ret

    .globl latx_avx_single_vunpcklpd_fault_xmm
    .type latx_avx_single_vunpcklpd_fault_xmm, @function
latx_avx_single_vunpcklpd_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vunpcklpd xmm2, xmm0, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vunpcklpd_fault_ymm
    .type latx_avx_single_vunpcklpd_fault_ymm, @function
latx_avx_single_vunpcklpd_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vunpcklpd ymm2, ymm0, ymmword ptr [rdi]
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
    .quad 0x7ff8000000000042, 0x7ff0000000000001
    .quad 0x7ff0000000000000, 0xfff0000000000000
    .quad 0x0000000000000001, 0x8000000000000001
    .quad 0x0000000000000000, 0x8000000000000000
input_b_unaligned:
    .byte 0x5a
    .quad 0x3ff0000000000000, 0xbff0000000000000
    .quad 0x7ff8000000000056, 0x7ff0000000000099
    .quad 0x0010000000000000, 0x8010000000000000
    .quad 0x7ff0000000000000, 0xfff0000000000000
input_b:
    .quad 0x3ff0000000000000, 0xbff0000000000000
    .quad 0x7ff8000000000056, 0x7ff0000000000099
    .quad 0x0010000000000000, 0x8010000000000000
    .quad 0x7ff0000000000000, 0xfff0000000000000

    .section .note.GNU-stack,"",@progbits
