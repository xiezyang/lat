/*
 * SPDX-FileCopyrightText: 2026 LAT Project Authors
 * SPDX-License-Identifier: GPL-2.0-only
 */
.intel_syntax noprefix
.text
.global _start

.macro EXTRACT op, reg, lane
    vmovdqu ymm\reg, [rip + input]
    \op xmm\reg, ymm\reg, \lane
    jmp .Lobserve\@
.Lobserve\@:
    vmovdqu [rip + out], ymm\reg
    cmp qword ptr [rip + out], (1 + 2 * \lane)
    jne fail
    cmp qword ptr [rip + out + 8], (2 + 2 * \lane)
    jne fail
    cmp qword ptr [rip + out + 16], 0
    jne fail
    cmp qword ptr [rip + out + 24], 0
    jne fail
.endm
.macro NARROW op, reg, floating
    vmovdqu ymm\reg, [rip + doubles]
    \op xmm\reg, ymm\reg
    vmovdqu [rip + out], ymm\reg
    .if \floating
      mov rax, 0x400000003f800000
      mov rdx, 0x4080000040400000
    .else
      mov rax, 0x0000000200000001
      mov rdx, 0x0000000400000003
    .endif
    cmp qword ptr [rip + out], rax
    jne fail
    cmp qword ptr [rip + out + 8], rdx
    jne fail
    cmp qword ptr [rip + out + 16], 0
    jne fail
    cmp qword ptr [rip + out + 24], 0
    jne fail
.endm
_start:
.irp reg,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    EXTRACT vextractf128,\reg,0
    EXTRACT vextractf128,\reg,1
    EXTRACT vextracti128,\reg,0
    EXTRACT vextracti128,\reg,1
    NARROW vcvtpd2ps,\reg,1
    NARROW vcvtpd2dq,\reg,0
    NARROW vcvttpd2dq,\reg,0
.endr
    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, 60
    syscall
.section .rodata
.p2align 5
input: .quad 1,2,3,4
doubles: .double 1.0,2.0,3.0,4.0
.bss
.p2align 5
out: .skip 32
.section .note.GNU-stack,"",@progbits
