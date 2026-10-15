/* SPDX-License-Identifier: GPL-2.0-only */
.text
.globl _start
_start:
    vmovdqu ones(%rip), %ymm11
    vpxor %ymm12, %ymm12, %ymm12
    mov $300000, %r15d
.Lagain:
    vmovdqu ones(%rip), %ymm1
    xor %edi, %edi
    call choose
    call check
    vmovdqu expected(%rip), %ymm1
    mov $1, %edi
    call choose
    call check
    dec %r15d
    jnz .Lagain
    xor %edi, %edi
.Lexit:
    mov $60, %eax
    syscall
.Lbad:
    mov $1, %edi
    jmp .Lexit

check:
    cmpq $-1, actual(%rip)
    jne .Lbad
    cmpq $-1, actual+8(%rip)
    jne .Lbad
    cmpq $0, actual+16(%rip)
    jne .Lbad
    cmpq $0, actual+24(%rip)
    jne .Lbad
    ret

.p2align 4
choose:
    test %edi, %edi
    jnz middle
    /* The branch target gives AOT a second entry into this instruction
     * sequence. Splitting the overlapping TBs leaves VPOR as the final
     * instruction of a TB that falls through, without a guest branch. */
    vpor %xmm11, %xmm12, %xmm1
middle:
    vmovdqu %ymm1, actual(%rip)
    ret

.section .rodata
.p2align 5
ones: .quad -1, -1, -1, -1
expected: .quad -1, -1, 0, 0
.bss
.p2align 5
actual: .skip 32
.section .note.GNU-stack,"",@progbits
