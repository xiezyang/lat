/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .extern latx_vmovhps_source_a
    .extern latx_vmovhps_source_b
    .extern latx_vmovhps_memory
    .extern latx_vmovhps_seed
    .extern latx_vmovhps_store_seed
    .extern latx_vmovhps_store_area

    .macro VMOVHPS_SAVE target
    vmovdqu ymmword ptr [rdi], ymm\target
    add rdi, 32
    .endm

    .globl latx_avx_single_vmovhps_run
    .type latx_avx_single_vmovhps_run, @function
latx_avx_single_vmovhps_run:
    /* Legal m64 load: two m32 elements come from memory. */
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovhps_seed]
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovhps_source_a]
    vmovhps xmm0, xmm1, qword ptr [rip + latx_vmovhps_memory + 3]
    VMOVHPS_SAVE 0

    /* Legal m64 load with destination/source register alias. */
    vmovdqu ymm0, ymmword ptr [rip + latx_vmovhps_source_b]
    vmovhps xmm0, xmm0, qword ptr [rip + latx_vmovhps_memory + 3]
    VMOVHPS_SAVE 0

    /* Legal two-operand m64 store. */
    vmovdqu ymm1, ymmword ptr [rip + latx_vmovhps_source_a]
    vmovdqu ymm2, ymmword ptr [rip + latx_vmovhps_store_seed]
    vmovhps qword ptr [rip + latx_vmovhps_store_area + 3], xmm1
    vmovdqu ymm2, ymmword ptr [rip + latx_vmovhps_store_area]
    VMOVHPS_SAVE 2

    /* A second store source proves store behavior is source-register based. */
    vmovdqu ymm15, ymmword ptr [rip + latx_vmovhps_source_b]
    vmovhps qword ptr [rip + latx_vmovhps_store_area + 3], xmm15
    vmovdqu ymm15, ymmword ptr [rip + latx_vmovhps_store_area]
    VMOVHPS_SAVE 15

    vzeroupper
    ret

    .globl latx_avx_single_vmovhps_fault_load
    .type latx_avx_single_vmovhps_fault_load, @function
latx_avx_single_vmovhps_fault_load:
    vmovdqu ymm15, ymmword ptr [rip + latx_vmovhps_seed]
    vmovhps xmm15, xmm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vmovhps_fault_store
    .type latx_avx_single_vmovhps_fault_store, @function
latx_avx_single_vmovhps_fault_store:
    vmovdqu ymm15, ymmword ptr [rip + latx_vmovhps_source_a]
    vmovhps qword ptr [rdi], xmm15
    ret

    .section .note.GNU-stack,"",@progbits
