.text
.global _start
_start:
    mov $-23, %rax
    mov $5, %rcx
    cqo
    idiv %rcx
    cmp $-4, %rax
    jne fail
    cmp $-3, %rdx
    jne fail
    mov $23, %eax
    mov $-5, %ecx
    cdq
    idiv %ecx
    cmp $-4, %eax
    jne fail
    cmp $3, %edx
    jne fail
    mov $5, %rax
    mov $7, %rcx
    mov $9, %rdx
    neg %rax
    cmovs %rcx, %rdx
    cmp $7, %rdx
    jne fail
    cmp $-5, %rax
    jne fail
    mov $1, %rax
    cmp $2, %rax
    sbb %rcx, %rcx
    setc %dl
    cmp $1, %dl
    jne fail
    cmp $-1, %rcx
    jne fail
    mov $2, %rax
    cmp $1, %rax
    sbb %rcx, %rcx
    setc %dl
    cmp $0, %dl
    jne fail
    cmp $0, %rcx
    jne fail
    xor %edi, %edi
    mov $60, %eax
    syscall
fail:
    mov $1, %edi
    mov $60, %eax
    syscall
.section .note.GNU-stack,"",@progbits
