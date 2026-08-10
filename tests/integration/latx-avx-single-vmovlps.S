/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .extern latx_vmovlps_source_a
    .extern latx_vmovlps_source_b
    .extern latx_vmovlps_memory
    .extern latx_vmovlps_seed
    .extern latx_vmovlps_store_seed
    .extern latx_vmovlps_store_area

    .macro VMOVLPS_SAVE target
    vmovdqu ymmword ptr [rdi], ymm\target
    add rdi, 32
    .endm

    .globl latx_avx_single_vmovlps_run
    .type latx_avx_single_vmovlps_run, @function
latx_avx_single_vmovlps_run:
    /* Legal m64 load: two m32 elements come from memory. */
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovlps_seed]
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovlps_source_a]
    vmovlps xmm0, xmm1, qword ptr [rip + latx_vmovlps_memory + 3]
    VMOVLPS_SAVE 0

    /* Legal m64 load with destination/source register alias. */
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovlps_source_b]
    vmovlps xmm0, xmm0, qword ptr [rip + latx_vmovlps_memory + 3]
    VMOVLPS_SAVE 0

    /* Legal two-operand m64 store. */
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovlps_source_a]
    vmovdqu ymm2, ymmword ptr [rip + latx_vmovlps_store_seed]
    vmovlps qword ptr [rip + latx_vmovlps_store_area + 3], xmm1
    vmovdqu ymm2, ymmword ptr [rip + latx_vmovlps_store_area]
    VMOVLPS_SAVE 2

    /* A second store source proves store behavior is source-register based. */
    vmovdqu ymm15, ymmword ptr [rip + latx_vmovlps_source_b]
    vmovlps qword ptr [rip + latx_vmovlps_store_area + 3], xmm15
    vmovdqu ymm15, ymmword ptr [rip + latx_vmovlps_store_area]
    VMOVLPS_SAVE 15

    vzeroupper
    ret

    .globl latx_avx_single_vmovlps_fault_load
    .type latx_avx_single_vmovlps_fault_load, @function
latx_avx_single_vmovlps_fault_load:
    vmovdqu ymm15, ymmword ptr [rip + latx_vmovlps_seed]
    vmovlps xmm15, xmm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vmovlps_fault_store
    .type latx_avx_single_vmovlps_fault_store, @function
latx_avx_single_vmovlps_fault_store:
    vmovdqu ymm15, ymmword ptr [rip + latx_vmovlps_source_a]
    vmovlps qword ptr [rdi], xmm15
    ret

    .section .note.GNU-stack,"",@progbits
