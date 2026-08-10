/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .extern wi1904_vmovlpd_source_a
    .extern wi1904_vmovlpd_source_b
    .extern wi1904_vmovlpd_seed

    .globl latx_avx_single_vmovlpd_run
    .type latx_avx_single_vmovlpd_run, @function
latx_avx_single_vmovlpd_run:
    /* Legal VEX.128 form: XMM register source plus m64 load. */
    vmovdqu ymm0, ymmword ptr [rip + wi1904_vmovlpd_seed]
    vmovdqu xmm1, xmmword ptr [rip + wi1904_vmovlpd_source_a]
    vmovlpd xmm0, xmm1, qword ptr [rsi]
    vmovdqu ymmword ptr [rdi], ymm0
    add rdi, 48

    /* Destination aliases the XMM register source. */
    vmovdqu ymm0, ymmword ptr [rip + wi1904_vmovlpd_seed]
    vmovdqu xmm0, xmmword ptr [rip + wi1904_vmovlpd_source_a]
    vmovlpd xmm0, xmm0, qword ptr [rsi + 8]
    vmovdqu ymmword ptr [rdi], ymm0
    add rdi, 48

    /* A second register alias uses the same legal m64 form. */
    vmovdqu ymm0, ymmword ptr [rip + wi1904_vmovlpd_seed]
    vmovdqu xmm2, xmmword ptr [rip + wi1904_vmovlpd_source_b]
    vmovlpd xmm2, xmm2, qword ptr [rsi + 16]
    vmovdqu ymmword ptr [rdi], ymm0
    add rdi, 48

    /* Legal m64 store form; the source YMM high half stays observable. */
    vmovdqu ymm0, ymmword ptr [rip + wi1904_vmovlpd_seed]
    vmovlpd qword ptr [rsi + 8], xmm0
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu xmm1, xmmword ptr [rsi]
    vmovdqu xmmword ptr [rdi + 32], xmm1

    vzeroupper
    ret

    .globl latx_avx_single_vmovlpd_fault_load
    .type latx_avx_single_vmovlpd_fault_load, @function
latx_avx_single_vmovlpd_fault_load:
    vmovdqu xmm1, xmmword ptr [rip + wi1904_vmovlpd_source_a]
    vmovlpd xmm15, xmm1, qword ptr [rdi]
    ret

    .globl latx_avx_single_vmovlpd_fault_store
    .type latx_avx_single_vmovlpd_fault_store, @function
latx_avx_single_vmovlpd_fault_store:
    vmovdqu xmm15, xmmword ptr [rip + wi1904_vmovlpd_source_a]
    vmovlpd qword ptr [rdi], xmm15
    ret

    .section .note.GNU-stack,"",@progbits
