/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .extern latx_vmovshdup_source_a
    .extern latx_vmovshdup_source_b
    .extern latx_vmovshdup_memory

    .macro SAVE target
    vmovdqu ymmword ptr [rdi], ymm\target
    add rdi, 32
    .endm

    .globl latx_avx_single_vmovshdup_run
    .type latx_avx_single_vmovshdup_run, @function
latx_avx_single_vmovshdup_run:
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovshdup_source_a]
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovshdup_source_b]
    vmovshdup xmm0, xmm1
    SAVE 0

    vmovdqu ymm0, ymmword ptr [rip + latx_vmovshdup_source_a]
    vmovshdup xmm0, xmm0
    SAVE 0

    vmovdqu ymm0, ymmword ptr [rip + latx_vmovshdup_source_a]
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovshdup_source_b]
    vmovshdup ymm0, ymm1
    SAVE 0

    vmovdqu ymm0, ymmword ptr [rip + latx_vmovshdup_source_a]
    vmovshdup ymm0, ymm0
    SAVE 0

    vmovdqu ymm0, ymmword ptr [rip + latx_vmovshdup_source_a]
    vmovshdup xmm0, xmmword ptr [rip + latx_vmovshdup_memory]
    SAVE 0

    vmovshdup ymm0, ymmword ptr [rip + latx_vmovshdup_memory]
    SAVE 0

    vzeroupper
    ret

    .globl latx_avx_single_vmovshdup_fault_xmm
latx_avx_single_vmovshdup_fault_xmm:
    vmovshdup xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovshdup_fault_ymm
latx_avx_single_vmovshdup_fault_ymm:
    vmovshdup ymm15, ymmword ptr [rdi]
    ret

    .section .note.GNU-stack,"",@progbits
