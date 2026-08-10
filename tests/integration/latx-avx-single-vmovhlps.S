/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .extern latx_vmovhlps_source_a
    .extern latx_vmovhlps_source_b
    .extern latx_vmovhlps_source_a_nan
    .extern latx_vmovhlps_source_b_nan
    .extern latx_vmovhlps_seed

    .macro VMOVHLPS_SAVE_CASE target
    vmovdqu ymmword ptr [rdi], ymm\target
    add rdi, 32
    .endm

    .globl latx_avx_single_vmovhlps_run
    .type latx_avx_single_vmovhlps_run, @function
latx_avx_single_vmovhlps_run:
    /* Distinct sources: low result comes from source 1 high lane. */
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovhlps_seed]
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovhlps_source_a]
    vmovdqu ymm2, ymmword ptr [rip + latx_vmovhlps_source_b]
    vmovhlps xmm0, xmm1, xmm2
    VMOVHLPS_SAVE_CASE 0

    /* Destination aliases source 1. */
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovhlps_source_a]
    vmovdqu ymm2, ymmword ptr [rip + latx_vmovhlps_source_b]
    vmovhlps xmm0, xmm0, xmm2
    VMOVHLPS_SAVE_CASE 0

    /* Destination aliases source 2. */
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovhlps_source_a_nan]
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovhlps_source_b_nan]
    vmovhlps xmm0, xmm1, xmm0
    VMOVHLPS_SAVE_CASE 0

    /* All three operands alias the destination. */
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovhlps_source_a_nan]
    vmovhlps xmm0, xmm0, xmm0
    VMOVHLPS_SAVE_CASE 0

    vzeroupper
    ret

    .section .note.GNU-stack,"",@progbits
