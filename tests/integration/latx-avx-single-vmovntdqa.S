/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .extern latx_vmovntdqa_memory
    .extern latx_vmovntdqa_seed

    .macro SAVE target
    vmovdqu ymmword ptr [rdi], ymm\target
    add rdi, 32
    .endm

    .globl latx_avx_single_vmovntdqa_run
    .type latx_avx_single_vmovntdqa_run, @function
latx_avx_single_vmovntdqa_run:
    /* XMM non-temporal load from an aligned m128 address. */
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovntdqa_seed]
    vmovntdqa xmm0, xmmword ptr [rip + latx_vmovntdqa_memory]
    SAVE 0

    /* YMM non-temporal load from the same aligned source. */
    vmovntdqa ymm0, ymmword ptr [rip + latx_vmovntdqa_memory]
    SAVE 0

    /* Ordinary loads are the observable-result reference for the hint. */
    vmovdqu xmm0, xmmword ptr [rip + latx_vmovntdqa_memory]
    SAVE 0
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovntdqa_memory]
    SAVE 0

    vzeroupper
    ret

    .globl latx_avx_single_vmovntdqa_fault_xmm
    .type latx_avx_single_vmovntdqa_fault_xmm, @function
latx_avx_single_vmovntdqa_fault_xmm:
    vmovntdqa xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovntdqa_fault_ymm
    .type latx_avx_single_vmovntdqa_fault_ymm, @function
latx_avx_single_vmovntdqa_fault_ymm:
    vmovntdqa ymm15, ymmword ptr [rdi]
    ret

    .section .note.GNU-stack,"",@progbits
