/* SPDX-License-Identifier: GPL-2.0-only
 * Borrowed key registers are not IR1 source operands.  Check both the AES
 * result and the restored register after a deferred VEX.128 upper clear.
 * For an all-zero state, AES encrypt/decrypt rounds produce repeated 63/52
 * bytes before XOR with the key (MixColumns preserves a constant column).
 */
.intel_syntax noprefix
.text
.global _start
.macro CASE op, width, d, s, borrowed, byteword
    vmovdqu ymm\borrowed, [rip + round_key]
    vpxor xmm\borrowed, xmm\borrowed, xmm\borrowed
    vpxor xmm\d, xmm\d, xmm\d
    vpxor xmm\s, xmm\s, xmm\s
    \op \width\()\d, \width\()\s, [rip + round_key]
    vmovdqu [rip + result], ymm\d
    vmovdqu [rip + saved], ymm\borrowed
    movabs rax, (\byteword ^ 0x0123456789abcdef)
    cmp [rip + result], rax
    jne fail
    movabs rax, (\byteword ^ 0xfedcba9876543210)
    cmp [rip + result + 8], rax
    jne fail
    .ifc \width,ymm
        movabs rax, (\byteword ^ 0x1122334455667788)
        cmp [rip + result + 16], rax
        jne fail
        movabs rax, (\byteword ^ 0x705b895ea61dc358)
        cmp [rip + result + 24], rax
        jne fail
    .else
        cmp qword ptr [rip + result + 16], 0
        jne fail
        cmp qword ptr [rip + result + 24], 0
        jne fail
    .endif
    cmp qword ptr [rip + saved], 0
    jne fail
    cmp qword ptr [rip + saved + 8], 0
    jne fail
    cmp qword ptr [rip + saved + 16], 0
    jne fail
    cmp qword ptr [rip + saved + 24], 0
    jne fail
.endm
_start:
    CASE vaesenclast,ymm,8,8,0,0x6363636363636363
    CASE vaesenclast,ymm,0,8,1,0x6363636363636363
    CASE vaesenclast,ymm,0,1,2,0x6363636363636363
    CASE vaesenclast,xmm,8,8,0,0x6363636363636363
    CASE vaesenclast,xmm,0,8,1,0x6363636363636363
    CASE vaesenclast,xmm,0,1,2,0x6363636363636363
    CASE vaesenc,ymm,8,8,0,0x6363636363636363
    CASE vaesenc,ymm,0,8,1,0x6363636363636363
    CASE vaesenc,ymm,0,1,2,0x6363636363636363
    CASE vaesenc,xmm,8,8,0,0x6363636363636363
    CASE vaesenc,xmm,0,8,1,0x6363636363636363
    CASE vaesenc,xmm,0,1,2,0x6363636363636363
    CASE vaesdeclast,ymm,8,8,0,0x5252525252525252
    CASE vaesdeclast,ymm,0,8,1,0x5252525252525252
    CASE vaesdeclast,ymm,0,1,2,0x5252525252525252
    CASE vaesdeclast,xmm,8,8,0,0x5252525252525252
    CASE vaesdeclast,xmm,0,8,1,0x5252525252525252
    CASE vaesdeclast,xmm,0,1,2,0x5252525252525252
    CASE vaesdec,ymm,8,8,0,0x5252525252525252
    CASE vaesdec,ymm,0,8,1,0x5252525252525252
    CASE vaesdec,ymm,0,1,2,0x5252525252525252
    CASE vaesdec,xmm,8,8,0,0x5252525252525252
    CASE vaesdec,xmm,0,8,1,0x5252525252525252
    CASE vaesdec,xmm,0,1,2,0x5252525252525252
    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, 60
    syscall
.section .rodata
.p2align 5
round_key:
    .quad 0x0123456789abcdef, 0xfedcba9876543210
    .quad 0x1122334455667788, 0x705b895ea61dc358
.bss
.p2align 5
result: .skip 32
saved: .skip 32
.section .note.GNU-stack,"",@progbits
