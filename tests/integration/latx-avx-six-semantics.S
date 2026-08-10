/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl avx_vmovdqa_ymm
    .type avx_vmovdqa_ymm, @function
avx_vmovdqa_ymm:
    vmovdqa ymm0, ymmword ptr [rdi]
    vmovdqa ymm1, ymm0
    vmovdqa ymmword ptr [rsi], ymm1
    ret

    .globl avx_vmovdqa_xmm
    .type avx_vmovdqa_xmm, @function
avx_vmovdqa_xmm:
    vmovdqa ymm1, ymmword ptr [rdi]
    vmovdqa xmm0, xmmword ptr [rdi]
    vmovdqa xmm1, xmm0
    vmovdqa xmmword ptr [rsi], xmm1
    vmovdqa ymmword ptr [rdx], ymm1
    ret

    .globl avx_vmovdqu_ymm
    .type avx_vmovdqu_ymm, @function
avx_vmovdqu_ymm:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu ymm1, ymm0
    vmovdqu ymmword ptr [rsi], ymm1
    ret

    .globl avx_vmovdqu_xmm
    .type avx_vmovdqu_xmm, @function
avx_vmovdqu_xmm:
    vmovdqu ymm1, ymmword ptr [rdi]
    vmovdqu xmm0, xmmword ptr [rdi]
    vmovdqu xmm1, xmm0
    vmovdqu xmmword ptr [rsi], xmm1
    vmovdqu ymmword ptr [rdx], ymm1
    ret

    .globl avx_vpxor_ymm_reg
    .type avx_vpxor_ymm_reg, @function
avx_vpxor_ymm_reg:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu ymm1, ymmword ptr [rsi]
    vpxor ymm2, ymm0, ymm1
    vmovdqu ymmword ptr [rdx], ymm2
    ret

    .globl avx_vpxor_ymm_mem_overlap
    .type avx_vpxor_ymm_mem_overlap, @function
avx_vpxor_ymm_mem_overlap:
    vmovdqu ymm0, ymmword ptr [rdi]
    vpxor ymm0, ymm0, ymmword ptr [rsi]
    vmovdqu ymmword ptr [rdx], ymm0
    ret

    .globl avx_vpxor_xmm_reg
    .type avx_vpxor_xmm_reg, @function
avx_vpxor_xmm_reg:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu xmm1, xmmword ptr [rsi]
    vpxor xmm0, xmm0, xmm1
    vmovdqu ymmword ptr [rdx], ymm0
    ret

    .globl avx_vpxor_xmm_mem
    .type avx_vpxor_xmm_mem, @function
avx_vpxor_xmm_mem:
    vmovdqu ymm0, ymmword ptr [rdi]
    vpxor xmm0, xmm0, xmmword ptr [rsi]
    vmovdqu ymmword ptr [rdx], ymm0
    ret

    .globl avx_vextracti128
    .type avx_vextracti128, @function
avx_vextracti128:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu ymm1, ymmword ptr [rdi]
    vextracti128 xmm1, ymm0, 0
    vmovdqu ymmword ptr [rsi], ymm1
    vmovdqu ymm2, ymmword ptr [rdi]
    vextracti128 xmm2, ymm0, 1
    vmovdqu ymmword ptr [rsi + 32], ymm2
    vextracti128 xmmword ptr [rsi + 64], ymm0, 0
    vextracti128 xmmword ptr [rsi + 80], ymm0, 1
    ret

    .globl avx_vzeroupper
    .type avx_vzeroupper, @function
avx_vzeroupper:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovdqu ymm15, ymmword ptr [rdi + 32]
    vzeroupper
    vmovdqu ymmword ptr [rsi], ymm0
    vmovdqu ymmword ptr [rsi + 32], ymm15
    ret

    .globl avx_vmovss_three
    .type avx_vmovss_three, @function
avx_vmovss_three:
    vmovdqu ymm1, ymmword ptr [rdi]
    vmovdqu ymm2, ymmword ptr [rsi]
    vmovss xmm0, xmm1, xmm2
    vmovdqu ymmword ptr [rdx], ymm0
    ret

    .globl avx_vmovss_load
    .type avx_vmovss_load, @function
avx_vmovss_load:
    vmovdqu ymm0, ymmword ptr [rdi]
    vmovss xmm0, dword ptr [rsi]
    vmovdqu ymmword ptr [rdx], ymm0
    ret

    .globl avx_vmovss_store
    .type avx_vmovss_store, @function
avx_vmovss_store:
    vmovdqu xmm0, xmmword ptr [rdi]
    vmovss dword ptr [rsi], xmm0
    ret

    .globl avx_cross_tb
    .type avx_cross_tb, @function
avx_cross_tb:
    vmovdqu ymm0, ymmword ptr [rdi]
    jmp .Lcross_tb_target
.Lcross_tb_target:
    vmovdqu ymmword ptr [rsi], ymm0
    ret

    .globl avx_across_syscall
    .type avx_across_syscall, @function
avx_across_syscall:
    vmovdqu ymm0, ymmword ptr [rdi]
    mov eax, 39
    syscall
    vmovdqu ymmword ptr [rsi], ymm0
    ret

    .section .note.GNU-stack,"",@progbits
