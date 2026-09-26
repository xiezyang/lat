.text
.global _start

.macro check width, reg, rhs, value, mask, expected
    movabs $\value, %rax
    movabs $\mask, %rcx
    and\width %\rhs, %\reg
    jne 1f
    setz %dl
    cmp $1, %dl
    jne fail
    jmp 2f
1:
    setz %dl
    cmp $0, %dl
    jne fail
2:
    movabs $\expected, %rcx
    cmp %rcx, %rax
    jne fail
.endm

_start:
    check b, al, cl, 0x123480, 0x7f, 0x123400
    check b, al, cl, 0x1234ff, 0x80, 0x123480
    check w, ax, cx, 0x12348000, 0x7fff, 0x12340000
    check w, ax, cx, 0x1234ffff, 0x8000, 0x12348000
    check l, eax, ecx, 0x1234567880000000, 0x7fffffff, 0
    check l, eax, ecx, 0xffffffffffffffff, 0x80000000, 0x80000000
    check q, rax, rcx, 0x8000000000000000, 0x7fffffffffffffff, 0
    check q, rax, rcx, 0xffffffffffffffff, 0x8000000000000000, 0x8000000000000000
    xor %edi, %edi
    mov $60, %eax
    syscall
fail:
    mov $1, %edi
    mov $60, %eax
    syscall
.section .note.GNU-stack,"",@progbits
