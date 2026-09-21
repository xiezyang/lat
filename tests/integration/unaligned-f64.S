/* Check all alignments, including loads crossing a mapped page boundary. */
.global _start
.text
_start:
    lea data(%rip), %rsi
    mov $2, %r8d
outer:
    xor %ecx, %ecx
next:
    movabs $0x3ff123456789abcd, %rax
    mov %rax, (%rsi,%rcx)
    movq (%rsi,%rcx), %xmm0
    movq %xmm0, %rdx
    cmp %rax, %rdx
    jne fail
    movsd (%rsi,%rcx), %xmm1
    movq %xmm1, %rdx
    cmp %rax, %rdx
    jne fail
    fldl (%rsi,%rcx)
    fstpl result(%rip)
    cmp result(%rip), %rax
    jne fail
    inc %ecx
    cmp $8, %ecx
    jne next
    lea data+65532(%rip), %rsi
    dec %r8d
    jne outer
    xor %edi, %edi
    jmp exit
fail:
    mov $1, %edi
exit:
    mov $60, %eax
    syscall
.bss
.balign 65536
data: .skip 131072
result: .skip 8
.section .note.GNU-stack,"",@progbits
