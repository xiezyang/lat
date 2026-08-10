/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_vpxor_run
    .type latx_avx_single_vpxor_run, @function
latx_avx_single_vpxor_run:
    .macro SAVE_RESULT
    vmovdqu ymmword ptr [rdi], ymm2
    add rdi, 32
    .endm

    /* XMM register, alias and unaligned-memory-source cases. */
    movdqu xmm0, xmmword ptr [rip + vpxor_zero]
    movdqu xmm1, xmmword ptr [rip + vpxor_ones]
    vpxor xmm2, xmm0, xmm1
    SAVE_RESULT
    vpxor xmm0, xmm0, xmm1
    movdqu xmm2, xmm0
    SAVE_RESULT
    vpxor xmm1, xmm0, xmm1
    movdqu xmm2, xmm1
    SAVE_RESULT
    movdqu xmm0, xmmword ptr [rip + vpxor_sign]
    vpxor xmm2, xmm0, xmmword ptr [rip + vpxor_random + 1]
    SAVE_RESULT

    movdqu xmm0, xmmword ptr [rip + vpxor_sign + 16]
    movdqu xmm1, xmmword ptr [rip + vpxor_random + 16]
    vpxor xmm2, xmm0, xmm1
    SAVE_RESULT
    vpxor xmm0, xmm0, xmm1
    movdqu xmm2, xmm0
    SAVE_RESULT
    vpxor xmm1, xmm0, xmm1
    movdqu xmm2, xmm1
    SAVE_RESULT
    movdqu xmm0, xmmword ptr [rip + vpxor_ones + 1]
    vpxor xmm2, xmm0, xmmword ptr [rip + vpxor_zero + 1]
    SAVE_RESULT

    /* YMM register, alias and unaligned-memory-source cases. */
    vmovdqu ymm0, ymmword ptr [rip + vpxor_zero]
    vmovdqu ymm1, ymmword ptr [rip + vpxor_ones]
    vpxor ymm2, ymm0, ymm1
    SAVE_RESULT
    vpxor ymm0, ymm0, ymm1
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vpxor ymm1, ymm0, ymm1
    vmovdqu ymm2, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + vpxor_sign]
    vpxor ymm2, ymm0, ymmword ptr [rip + vpxor_random + 1]
    SAVE_RESULT

    vmovdqu ymm0, ymmword ptr [rip + vpxor_random]
    vmovdqu ymm1, ymmword ptr [rip + vpxor_sign]
    vpxor ymm2, ymm0, ymm1
    SAVE_RESULT
    vpxor ymm0, ymm0, ymm1
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vpxor ymm1, ymm0, ymm1
    vmovdqu ymm2, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + vpxor_ones + 1]
    vpxor ymm2, ymm0, ymmword ptr [rip + vpxor_zero + 1]
    SAVE_RESULT

    vzeroupper

    ret

    .globl latx_avx_single_vpxor_fault_xmm
    .type latx_avx_single_vpxor_fault_xmm, @function
latx_avx_single_vpxor_fault_xmm:
    movdqu xmm0, xmmword ptr [rip + vpxor_random]
    vpxor xmm2, xmm0, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vpxor_fault_ymm
    .type latx_avx_single_vpxor_fault_ymm, @function
latx_avx_single_vpxor_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + vpxor_random]
    vpxor ymm2, ymm0, ymmword ptr [rdi]
    vzeroupper
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .data
    .balign 32
vpxor_zero:
    .zero 32
vpxor_ones:
    .fill 32, 1, 0xff
vpxor_sign:
    .quad 0x8000000000000000, 0x7fffffffffffffff
    .quad 0xffffffffffffffff, 0x0000000000000000
vpxor_random:
    .quad 0x0011223344556677, 0x8899aabbccddeeff
    .quad 0x1021324354657687, 0x98a9bacbdcedfe0f

    .section .note.GNU-stack,"",@progbits
