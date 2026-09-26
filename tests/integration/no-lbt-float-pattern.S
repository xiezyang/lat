.text
.global _start
_start:
    mov $0x3f800000, %eax
    movd %eax, %xmm0
    mov $0x40000000, %eax
    movd %eax, %xmm1
    comiss %xmm1, %xmm0
    jb 1f
    jmp fail
1:
    setc %dl
    cmp $1, %dl
    jne fail
    ucomiss %xmm0, %xmm0
    je 2f
    jmp fail
2:
    movabs $0x3ff0000000000000, %rax
    movq %rax, %xmm0
    movabs $0x4000000000000000, %rax
    movq %rax, %xmm1
    comisd %xmm0, %xmm1
    ja 3f
    jmp fail
3:
    movabs $0x7ff8000000000000, %rax
    movq %rax, %xmm1
    ucomisd %xmm1, %xmm0
    jb 4f
    jmp fail
4:
    setp %dl
    cmp $1, %dl
    jne fail
    ucomisd %xmm1, %xmm0
    jge 5f
    jmp fail
5:
    ucomisd %xmm1, %xmm0
    jl fail
    ucomisd %xmm0, %xmm0
    mov $7, %eax
    je 6f
    jmp fail
6:
    comiss %xmm0, %xmm0
    nop
    je 7f
    jmp fail
7:
    ucomiss %xmm0, %xmm0
    nop
    je 8f
    jmp fail
8:
    comisd %xmm0, %xmm0
    nop
    je 9f
    jmp fail
9:
    ucomisd %xmm0, %xmm0
    seta %dl
    cmp $0, %dl
    jne fail
    xor %edi, %edi
    mov $60, %eax
    syscall
fail:
    mov $1, %edi
    mov $60, %eax
    syscall
.section .note.GNU-stack,"",@progbits
