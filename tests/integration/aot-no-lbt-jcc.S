.equ __NR_exit, 60

.section .rodata
.align 8
.Lvalues:
    .quad 1
    .quad 0

.section .text
.global _start
.type _start, @function
_start:
    lea .Lvalues(%rip), %rdi

.Lloop:
    add $8, %rdi
    cmpq $0, -8(%rdi)
    jne .Lloop

    xor %edi, %edi
    mov $__NR_exit, %eax
    syscall
.size _start, .-_start
