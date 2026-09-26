.text
.global _start
_start:
    lea data(%rip), %rdi
    pcmpeqd %xmm0, %xmm0
    mov $200, %ecx
1:
    movaps %xmm0, 0(%rdi)
    movaps %xmm0, 16(%rdi)
    movaps %xmm0, 32(%rdi)
    movaps %xmm0, 48(%rdi)
    loop 1b
    mov $8, %ecx
2:
    cmpq $-1, (%rdi)
    jne fail
    add $8, %rdi
    loop 2b
    xor %edi, %edi
    mov $60, %eax
    syscall
fail:
    mov $1, %edi
    mov $60, %eax
    syscall
.bss
.balign 4096
data:
    .skip 4096
.section .note.GNU-stack,"",@progbits
