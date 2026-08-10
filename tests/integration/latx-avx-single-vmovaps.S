/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_vmovaps_run
    .type latx_avx_single_vmovaps_run, @function
latx_avx_single_vmovaps_run:
    push r12
    mov rdx, rsi

    vmovaps xmm0, xmmword ptr [rsi]
    vmovaps xmmword ptr [rdi], xmm0
    vmovaps xmm15, xmm0
    vmovaps xmmword ptr [rdi + 16], xmm15
    vmovaps xmm15, xmm15
    vmovaps xmmword ptr [rdi + 32], xmm15

    mov r12, 16
    vmovaps xmm0, xmmword ptr [rsi + r12 * 2]
    vmovaps xmmword ptr [rdi + 48], xmm0
    vmovaps xmm0, xmmword ptr [rsi + 64]
    vmovaps xmmword ptr [rdi + 64], xmm0
    vmovaps xmm0, xmmword ptr [rsi + 4096]
    vmovaps xmmword ptr [rdi + 80], xmm0
    vmovaps xmm0, xmmword ptr [rip + vmovaps_pattern_xmm]
    vmovaps xmmword ptr [rdi + 96], xmm0

    movdqu xmm1, xmmword ptr [rip + vmovaps_pattern_xmm]
    vmovaps xmmword ptr [rdx + 512], xmm1
    movdqu xmm2, xmmword ptr [rdx + 496]
    movdqu xmmword ptr [rdi + 112], xmm2
    movdqu xmm2, xmmword ptr [rdx + 512]
    movdqu xmmword ptr [rdi + 128], xmm2
    movdqu xmm2, xmmword ptr [rdx + 528]
    movdqu xmmword ptr [rdi + 144], xmm2

    vmovaps ymm0, ymmword ptr [rsi]
    vmovaps ymmword ptr [rdi + 160], ymm0
    vmovaps ymm15, ymm0
    vmovaps ymmword ptr [rdi + 192], ymm15
    vmovaps ymm15, ymm15
    vmovaps ymmword ptr [rdi + 224], ymm15
    vmovaps ymm0, ymmword ptr [rsi + r12 * 2]
    vmovaps ymmword ptr [rdi + 256], ymm0
    vmovaps ymm0, ymmword ptr [rsi + 64]
    vmovaps ymmword ptr [rdi + 288], ymm0
    vmovaps ymm0, ymmword ptr [rsi + 4096]
    vmovaps ymmword ptr [rdi + 320], ymm0
    vmovaps ymm0, ymmword ptr [rip + vmovaps_pattern_ymm]
    vmovaps ymmword ptr [rdi + 352], ymm0

    vmovaps ymm1, ymmword ptr [rip + vmovaps_pattern_ymm]
    vmovaps ymmword ptr [rdx + 1024], ymm1
    movdqu xmm2, xmmword ptr [rdx + 1008]
    movdqu xmmword ptr [rdi + 384], xmm2
    movdqu xmm2, xmmword ptr [rdx + 1024]
    movdqu xmmword ptr [rdi + 400], xmm2
    movdqu xmm2, xmmword ptr [rdx + 1040]
    movdqu xmmword ptr [rdi + 416], xmm2
    movdqu xmm2, xmmword ptr [rdx + 1056]
    movdqu xmmword ptr [rdi + 432], xmm2

    vmovaps xmm0, xmmword ptr [rsi + 4080]
    vmovaps xmmword ptr [rdi + 448], xmm0
    vmovaps ymm0, ymmword ptr [rsi + 4064]
    vmovaps ymmword ptr [rdi + 480], ymm0

    movdqu xmm1, xmmword ptr [rip + vmovaps_pattern_xmm]
    vmovaps xmmword ptr [rdx + 4080], xmm1
    movdqu xmm2, xmmword ptr [rdx + 4080]
    movdqu xmmword ptr [rdi + 512], xmm2
    vmovaps ymm1, ymmword ptr [rip + vmovaps_pattern_ymm]
    vmovaps ymmword ptr [rdx + 4064], ymm1
    movdqu xmm2, xmmword ptr [rdx + 4064]
    movdqu xmmword ptr [rdi + 528], xmm2
    movdqu xmm2, xmmword ptr [rdx + 4080]
    movdqu xmmword ptr [rdi + 544], xmm2

    pop r12
    ret

    .globl latx_avx_single_vmovaps_trace
    .type latx_avx_single_vmovaps_trace, @function
latx_avx_single_vmovaps_trace:
    movdqu xmm0, xmmword ptr [rip + vmovaps_pattern_xmm]
    mov ecx, 2
.Lvmovaps_trace_loop:
    vmovaps xmm15, xmm0
    dec ecx
    jne .Lvmovaps_trace_loop
    ret

    .globl latx_avx_single_vmovaps_fault_xmm_load
    .type latx_avx_single_vmovaps_fault_xmm_load, @function
latx_avx_single_vmovaps_fault_xmm_load:
    movdqu xmm15, xmmword ptr [rip + vmovaps_fault_initial]
    vmovaps xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovaps_fault_xmm_store
    .type latx_avx_single_vmovaps_fault_xmm_store, @function
latx_avx_single_vmovaps_fault_xmm_store:
    movdqu xmm15, xmmword ptr [rip + vmovaps_fault_initial]
    vmovaps xmmword ptr [rdi], xmm15
    ret

    .globl latx_avx_single_vmovaps_fault_ymm_load
    .type latx_avx_single_vmovaps_fault_ymm_load, @function
latx_avx_single_vmovaps_fault_ymm_load:
    vmovaps ymm15, ymmword ptr [rip + vmovaps_pattern_ymm]
    vmovaps ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovaps_fault_ymm_store
    .type latx_avx_single_vmovaps_fault_ymm_store, @function
latx_avx_single_vmovaps_fault_ymm_store:
    vmovaps ymm15, ymmword ptr [rip + vmovaps_pattern_ymm]
    vmovaps ymmword ptr [rdi], ymm15
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vmovaps_pattern_ymm:
    .quad 0x0706050403020100, 0x0f0e0d0c0b0a0908
    .quad 0x1716151413121110, 0x1f1e1d1c1b1a1918
vmovaps_pattern_xmm:
    .quad 0x8877665544332211, 0x0f1e2d3c4b5a6978
vmovaps_fault_initial:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850

    .section .note.GNU-stack,"",@progbits
