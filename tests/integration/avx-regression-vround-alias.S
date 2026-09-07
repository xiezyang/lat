/*
 * SPDX-FileCopyrightText: 2026 LAT Project Authors
 * SPDX-License-Identifier: GPL-2.0-only
 */
.intel_syntax noprefix
.text
.global _start
_start:

    vmovdqu ymm0, [rip + input]
    vroundps ymm0, ymm0, 0x0b
    vmovdqu [rip + out], ymm0
    vmovdqu ymm0, [rip + input]
    vroundps ymm1, ymm0, 0x0b
    vmovdqu [rip + out + 32], ymm1
    vmovdqu xmm0, [rip + input]
    vroundps xmm0, xmm0, 0x0b
    vmovdqu [rip + out + 64], xmm0
    vmovdqu xmm0, [rip + input]
    vroundps xmm1, xmm0, 0x0b
    vmovdqu [rip + out + 80], xmm1

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
input: .long 0x4b800000,0x7fc12345,0x7f812345,0x3fc00000,0xcb800000,0xffc12345,0x80000000,0xbf400000
.bss
.p2align 5
out: .skip 96
.section .note.GNU-stack,"",@progbits

.section .rodata
.p2align 4
.Lreview_expected:
    .long 0x4b800000, 0x7fc12345, 0x7fc12345, 0x3f800000
    .long 0xcb800000, 0xffc12345, 0x80000000, 0x80000000
    .long 0x4b800000, 0x7fc12345, 0x7fc12345, 0x3f800000
    .long 0xcb800000, 0xffc12345, 0x80000000, 0x80000000
    .long 0x4b800000, 0x7fc12345, 0x7fc12345, 0x3f800000
    .long 0x4b800000, 0x7fc12345, 0x7fc12345, 0x3f800000
