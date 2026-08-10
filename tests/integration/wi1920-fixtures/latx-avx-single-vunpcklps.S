/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro SAVE_RESULT
    vmovdqu ymmword ptr [rdi], ymm2
    add rdi, 32
    .endm

    .globl latx_avx_single_vunpcklps_run
    .type latx_avx_single_vunpcklps_run, @function
latx_avx_single_vunpcklps_run:
    /* XMM register source, destination aliases src1, src2, and both. */
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vunpcklps xmm2, xmm0, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vunpcklps xmm0, xmm0, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vunpcklps xmm1, xmm0, xmm1
    vmovdqu xmm2, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vunpcklps xmm0, xmm0, xmm0
    vmovdqu xmm2, xmm0
    SAVE_RESULT

    /* XMM memory source, with destination/src1 aliases. */
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vunpcklps xmm2, xmm0, xmmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vunpcklps xmm0, xmm0, xmmword ptr [rip + input_b_unaligned]
    vmovdqu xmm2, xmm0
    SAVE_RESULT

    /* YMM register source, covering both independent 128-bit lanes. */
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vunpcklps ymm2, ymm0, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vunpcklps ymm0, ymm0, ymm1
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vunpcklps ymm1, ymm0, ymm1
    vmovdqu ymm2, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vunpcklps ymm0, ymm0, ymm0
    vmovdqu ymm2, ymm0
    SAVE_RESULT

    /* YMM memory source, including an unaligned legal address. */
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vunpcklps ymm2, ymm0, ymmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vunpcklps ymm0, ymm0, ymmword ptr [rip + input_b_unaligned]
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vzeroupper
    ret

    .globl latx_avx_single_vunpcklps_fault_xmm
    .type latx_avx_single_vunpcklps_fault_xmm, @function
latx_avx_single_vunpcklps_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vunpcklps xmm2, xmm0, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vunpcklps_fault_ymm
    .type latx_avx_single_vunpcklps_fault_ymm, @function
latx_avx_single_vunpcklps_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vunpcklps ymm2, ymm0, ymmword ptr [rdi]
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
    .long 0x7fc01234, 0x7fa00042, 0x00000000, 0x80000000
    .long 0x7f800000, 0xff800000, 0x00000001, 0x80000001
input_b_unaligned:
    .byte 0x5a
    .long 0x3f800000, 0xbf800000, 0x7fc05678, 0x7fa00099
    .long 0x00800000, 0x80800000, 0x7f800000, 0xff800000
input_b:
    .long 0x3f800000, 0xbf800000, 0x7fc05678, 0x7fa00099
    .long 0x00800000, 0x80800000, 0x7f800000, 0xff800000

    .section .note.GNU-stack,"",@progbits
