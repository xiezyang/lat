/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VLDDQU_SAVE offset
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vlddqu_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vlddqu_xsave_area + 160 + 15 * 16]
    movdqu xmmword ptr [rdi + \offset], xmm14
    movdqu xmm14, xmmword ptr [rip + vlddqu_xsave_area + 576 + 15 * 16]
    movdqu xmmword ptr [rdi + \offset + 16], xmm14
    .endm

    .globl latx_avx_single_vlddqu_run
    .type latx_avx_single_vlddqu_run, @function
latx_avx_single_vlddqu_run:
    vlddqu xmm15, xmmword ptr [rsi + 1]
    VLDDQU_SAVE 0
    vlddqu xmm15, xmmword ptr [rsi + 15]
    VLDDQU_SAVE 32
    vlddqu xmm15, xmmword ptr [rsi + 16]
    VLDDQU_SAVE 64
    vlddqu xmm15, xmmword ptr [rsi + 31]
    VLDDQU_SAVE 96
    vlddqu xmm15, xmmword ptr [rsi + 128]
    VLDDQU_SAVE 128
    vlddqu xmm15, xmmword ptr [rsi + 255]
    VLDDQU_SAVE 160
    vlddqu xmm15, xmmword ptr [rsi + 1024]
    VLDDQU_SAVE 192
    vlddqu xmm15, xmmword ptr [rsi + 2047]
    VLDDQU_SAVE 224

    vlddqu ymm15, ymmword ptr [rsi + 1]
    VLDDQU_SAVE 256
    vlddqu ymm15, ymmword ptr [rsi + 15]
    VLDDQU_SAVE 288
    vlddqu ymm15, ymmword ptr [rsi + 16]
    VLDDQU_SAVE 320
    vlddqu ymm15, ymmword ptr [rsi + 31]
    VLDDQU_SAVE 352
    vlddqu ymm15, ymmword ptr [rsi + 128]
    VLDDQU_SAVE 384
    vlddqu ymm15, ymmword ptr [rsi + 255]
    VLDDQU_SAVE 416
    vlddqu ymm15, ymmword ptr [rsi + 4065]
    VLDDQU_SAVE 448
    vlddqu ymm15, ymmword ptr [rsi + 4079]
    VLDDQU_SAVE 480
    ret

    .globl latx_avx_single_vlddqu_fault_xmm
    .type latx_avx_single_vlddqu_fault_xmm, @function
latx_avx_single_vlddqu_fault_xmm:
    vlddqu xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vlddqu_fault_ymm
    .type latx_avx_single_vlddqu_fault_ymm, @function
latx_avx_single_vlddqu_fault_ymm:
    vlddqu ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vlddqu_rt_sigreturn
    .type latx_avx_single_vlddqu_rt_sigreturn, @function
latx_avx_single_vlddqu_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .bss
    .balign 64
vlddqu_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
