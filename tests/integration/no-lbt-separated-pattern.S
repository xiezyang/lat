.text
.global _start
_start:
    mov $3, %eax
    cmp $4, %eax
    mov $10, %eax
    nop
    jb 1f
    jmp fail
1:
    setc %dl
    cmp $1, %dl
    jne fail
    mov $0x80000000, %eax
    cmp $1, %eax
    lea 1(%rax), %rax
    jl 2f
    jmp fail
2:
    xor %eax, %eax
    test %eax, %eax
    mov $1, %eax
    je 3f
    jmp fail
3:
    setz %dl
    cmp $1, %dl
    jne fail
    mov $0x80, %eax
    test %al, %al
    mov $0, %eax
    js 4f
    jmp fail
4:
    mov $0x8000, %eax
    btw $31, %ax
    mov $0, %eax
    jb 5f
    jmp fail
5:
    setc %dl
    cmp $1, %dl
    jne fail
    xor %edi, %edi
    mov $60, %eax
    syscall
fail:
    mov $1, %edi
    mov $60, %eax
    syscall
.section .note.GNU-stack,"",@progbits
