/*
 * SPDX-FileCopyrightText: 2026 LAT Project Authors
 * SPDX-License-Identifier: GPL-2.0-only
 */
.intel_syntax noprefix
.text
.global _start
_start:

    vmovdqu ymm0, [rip + input]
    vextractf128 xmm0, ymm0, 1
    vmovdqu [rip + out], ymm0
    vmovdqu ymm0, [rip + input]
    vextracti128 xmm0, ymm0, 0
    vmovdqu [rip + out + 32], ymm0
    vmovdqu ymm0, [rip + input]
    vextractf128 xmm1, ymm0, 1
    vmovdqu [rip + out + 64], ymm1

    lea rsi, [rip + out]
    lea rdi, [rip + .Lreview_expected]
    mov ecx, 24
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
input: .quad 1,2,3,4
.bss
.p2align 5
out: .skip 96
.section .note.GNU-stack,"",@progbits

.section .rodata
.p2align 4
.Lreview_expected:
    .long 0x00000003, 0x00000000, 0x00000004, 0x00000000
    .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
    .long 0x00000001, 0x00000000, 0x00000002, 0x00000000
    .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
    .long 0x00000003, 0x00000000, 0x00000004, 0x00000000
    .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
