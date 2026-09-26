.text
.global _start

/* Check wrapped indices, both branch directions and CF in the successor. */
.macro check width, reg, bitreg, value, index, expected
    movabs $\value, %rax
    mov $\index, %rcx
    bt\width %\bitreg, %\reg
    jb 1f
    setc %dl
    cmp $0, %dl
    jne fail
    mov $0, %edx
    jmp 2f
1:
    setc %dl
    cmp $1, %dl
    jne fail
    mov $1, %edx
2:
    cmp $\expected, %edx
    jne fail
    movabs $\value, %rax
    bt\width $\index, %\reg
    jae 3f
    setc %dl
    cmp $1, %dl
    jne fail
    mov $1, %edx
    jmp 4f
3:
    setc %dl
    cmp $0, %dl
    jne fail
    mov $0, %edx
4:
    cmp $\expected, %edx
    jne fail
.endm

_start:
    check w, ax, cx, 0x8000, 15, 1
    check w, ax, cx, 0x8000, 31, 1
    check w, ax, cx, 0x8000, 16, 0
    check l, eax, ecx, 0x80000000, 31, 1
    check l, eax, ecx, 0x80000000, 63, 1
    check l, eax, ecx, 0x80000000, 32, 0
    check q, rax, rcx, 0x8000000000000000, 63, 1
    check q, rax, rcx, 0x8000000000000000, 127, 1
    check q, rax, rcx, 0x8000000000000000, 64, 0
    xor %edi, %edi
    mov $60, %eax
    syscall
fail:
    mov $1, %edi
    mov $60, %eax
    syscall
.section .note.GNU-stack,"",@progbits
