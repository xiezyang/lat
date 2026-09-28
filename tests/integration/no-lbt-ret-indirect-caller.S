.section .text
.global _start
_start:
    mov $1, %rdi
    mov $2, %rsi
    call callee
    xor %eax, %eax

    lea callee(%rip), %rax
    call *%rax
    pushfq
    pop %rax
    and $0x8d5, %eax
    cmp $0x95, %eax
    jne fail
    xor %edi, %edi
    jmp exit

callee:
    cmp %rsi, %rdi
    ret

fail:
    mov $1, %edi
exit:
    mov $60, %eax
    syscall
.section .note.GNU-stack,"",@progbits
