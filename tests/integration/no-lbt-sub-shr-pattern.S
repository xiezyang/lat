.text
.global _start
_start:
    mov $0x123400, %eax
    subb $1, %al
    jb 10f
    jmp fail
10:
    pushfq
    pop %rdx
    and $0x8d5, %edx
    cmp $0x95, %edx
    jne fail
    cmp $0x1234ff, %eax
    jne fail
    mov $0x80000000, %eax
    sub $1, %eax
    jl 1f
    jmp fail
1:
    seto %dl
    cmp $1, %dl
    jne fail
    cmp $0x7fffffff, %eax
    jne fail
    xor %eax, %eax
    sub $1, %eax
    jb 2f
    jmp fail
2:
    setc %dl
    cmp $1, %dl
    jne fail
    mov $5, %eax
    sub %eax, %eax
    je 3f
    jmp fail
3:
    setz %dl
    cmp $1, %dl
    jne fail
    mov $0x123480, %eax
    shrb $7, %al
    jne 4f
    jmp fail
4:
    setz %dl
    cmp $0, %dl
    jne fail
    cmp $0x123401, %eax
    jne fail
    mov $1, %eax
    shr $1, %eax
    jne fail
    setc %dl
    cmp $1, %dl
    jne fail
    xor %eax, %eax
    mov $1, %eax
    shr $32, %eax
    jne fail
    cmp $1, %eax
    jne fail
    xor %edi, %edi
    mov $60, %eax
    syscall
fail:
    mov $1, %edi
    mov $60, %eax
    syscall
.section .note.GNU-stack,"",@progbits
