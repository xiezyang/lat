/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text
    .extern latx_vmovsldup_source_a
    .extern latx_vmovsldup_source_b
    .extern latx_vmovsldup_memory

    .macro SAVE target
    vmovdqu ymmword ptr [rdi], ymm\target
    add rdi, 32
    .endm

    .globl latx_avx_single_vmovsldup_run
    .type latx_avx_single_vmovsldup_run, @function
latx_avx_single_vmovsldup_run:
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovsldup_source_a]
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovsldup_source_b]
    vmovsldup xmm0, xmm1
    SAVE 0

    vmovdqu ymm0, ymmword ptr [rip + latx_vmovsldup_source_a]
    vmovsldup xmm0, xmm0
    SAVE 0

    vmovdqu ymm0, ymmword ptr [rip + latx_vmovsldup_source_a]
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovsldup_source_b]
    vmovsldup ymm0, ymm1
    SAVE 0

    vmovdqu ymm0, ymmword ptr [rip + latx_vmovsldup_source_a]
    vmovsldup ymm0, ymm0
    SAVE 0

    vmovdqu ymm0, ymmword ptr [rip + latx_vmovsldup_source_a]
    vmovsldup xmm0, xmmword ptr [rip + latx_vmovsldup_memory]
    SAVE 0

    vmovsldup ymm0, ymmword ptr [rip + latx_vmovsldup_memory]
    SAVE 0

    vzeroupper
    ret

    .globl latx_avx_single_vmovsldup_fault_xmm
latx_avx_single_vmovsldup_fault_xmm:
    vmovsldup xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovsldup_fault_ymm
latx_avx_single_vmovsldup_fault_ymm:
    vmovsldup ymm15, ymmword ptr [rdi]
    ret

    .section .note.GNU-stack,"",@progbits
