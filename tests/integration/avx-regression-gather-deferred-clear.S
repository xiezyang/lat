/* SPDX-License-Identifier: GPL-2.0-only */
.intel_syntax noprefix
.text
.global _start
_start:
    vmovdqu ymm1, [rip + ones]
    vpmovsxbd xmm3, xmm1
    vmovdqu ymm12, [rip + zeros]
    vpxor xmm8, xmm8, xmm8
    lea rdi, [rip + data]
    vgatherqpd ymm3, [rdi + ymm12 * 8], ymm8
    vmovdqu [rip + result], ymm3
    cmp qword ptr [rip + result], -1
    jne fail
    cmp qword ptr [rip + result + 8], -1
    jne fail
    cmp qword ptr [rip + result + 16], 0
    jne fail
    cmp qword ptr [rip + result + 24], 0
    jne fail
    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, 60
    syscall
.section .rodata
.balign 32
ones: .quad -1, -1, -1, -1
zeros: .quad 0, 0, 0, 0
data: .quad 0, 0, 0, 0
.bss
.balign 32
result: .skip 32
.section .note.GNU-stack,"",@progbits
