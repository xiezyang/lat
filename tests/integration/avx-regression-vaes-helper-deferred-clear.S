/* SPDX-License-Identifier: GPL-2.0-only */
.intel_syntax noprefix
.text
.global _start
.macro PREPARE_BORROWED_XMM4
    vmovdqu ymm1, [rip + ones]
    vpmovsxbd xmm4, xmm1
.endm
.macro CHECK_XMM4_HIGH
    vmovdqu [rip + result], ymm4
    cmp qword ptr [rip + result + 16], 0
    jne fail
    cmp qword ptr [rip + result + 24], 0
    jne fail
.endm
_start:
    /* XMM9 makes the memory helper borrow XMM4. */
    PREPARE_BORROWED_XMM4
    vaesimc xmm9, [rip + round_key]
    CHECK_XMM4_HIGH

    PREPARE_BORROWED_XMM4
    vaeskeygenassist xmm9, [rip + round_key], 1
    CHECK_XMM4_HIGH

    PREPARE_BORROWED_XMM4
    vpcmpistri xmm3, [rip + round_key], 0
    CHECK_XMM4_HIGH

    /* A memory F16C destination borrows (source + 1) & 7: XMM7. */
    vmovdqu ymm1, [rip + ones]
    vpmovsxbd xmm7, xmm1
    vpxor xmm14, xmm14, xmm14
    vcvtps2ph [rip + converted], xmm14, 0
    vmovdqu [rip + result], ymm7
    cmp qword ptr [rip + result], -1
    jne fail
    cmp qword ptr [rip + result + 8], -1
    jne fail
    cmp qword ptr [rip + result + 16], 0
    jne fail
    cmp qword ptr [rip + result + 24], 0
    jne fail
    cmp qword ptr [rip + converted], 0
    jne fail

    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, 60
    syscall
.section .rodata
.p2align 5
ones: .quad -1, -1, -1, -1
round_key:
    .quad 0x0123456789abcdef, 0xfedcba9876543210
.bss
.p2align 5
result: .skip 32
converted: .skip 8
.section .note.GNU-stack,"",@progbits
