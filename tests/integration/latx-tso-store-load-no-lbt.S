.equ __NR_exit, 60

.section .text
.global _start
.type _start, @function
_start:
    sub $32, %rsp
    xor %eax, %eax
    movl %eax, 0(%rsp)
    movl %eax, 4(%rsp)

.global tso_store_load_site
tso_store_load_site:
    movl $1, 0(%rsp)
    movl 4(%rsp), %eax
    movl %eax, 8(%rsp)
    cmpl $0, 8(%rsp)
    jne .Lfail
    xor %edi, %edi
    jmp .Lexit

.Lfail:
    mov $1, %edi
.Lexit:
    mov $__NR_exit, %eax
    syscall
.size _start, .-_start
