/*
 * SPDX-FileCopyrightText: 2026 LAT Project Authors
 * SPDX-License-Identifier: GPL-2.0-only
 */
.intel_syntax noprefix
.text
.global _start
_start:

    vmovaps xmm4, [rip + seven]
    vmovaps xmm2, [rip + two]
    vmovaps xmm3, [rip + three]
    vmulss xmm4, xmm4, xmm2
    vfmadd231ss xmm4, xmm4, xmm3
    vmovdqu [rip + out], xmm4

    lea rsi, [rip + out]
    lea rdi, [rip + .Lreview_expected]
    mov ecx, 4
.Lreview_check:
    mov eax, [rsi]
    cmp eax, [rdi]
    jne .Lreview_fail
    add rsi, 4
    add rdi, 4
    dec ecx
    jne .Lreview_check
    xor edi, edi
    jmp .Lreview_exit
.Lreview_fail:
    mov edi, 1
.Lreview_exit:
    mov eax, 60
    syscall
.section .rodata
.p2align 5
seven: .float 7,8,9,10
two: .float 2,2,2,2
three: .float 3,3,3,3
.bss
.p2align 5
out: .skip 16
.section .note.GNU-stack,"",@progbits

.section .rodata
.p2align 4
.Lreview_expected:
    .long 0x42600000, 0x41000000, 0x41100000, 0x41200000
