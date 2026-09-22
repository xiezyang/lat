.global _start
.text
_start:
    movabs $0x1122334455667788, %r8
    movabs $0x8877665544332211, %r12
    movdqu sentinel(%rip), %xmm0
    movdqu sentinel(%rip), %xmm15
    stc
    fldl value(%rip)
    pushfq
    pop %r10
    test $1, %r10
    jz fail
    movq %mm7, %rax
    cmp value(%rip), %rax
    jne fail
    movq %rax, %mm0
    fstpt extended(%rip)
    fldt extended(%rip)
    movq %mm7, %rax
    cmp value(%rip), %rax
    jne fail
    movabs $0x1122334455667788, %rax
    cmp %rax, %r8
    jne fail
    movabs $0x8877665544332211, %rax
    cmp %rax, %r12
    jne fail
    movdqu %xmm0, result(%rip)
    mov sentinel(%rip), %rax
    cmp result(%rip), %rax
    jne fail
    mov sentinel+8(%rip), %rax
    cmp result+8(%rip), %rax
    jne fail
    movdqu %xmm15, result(%rip)
    mov sentinel(%rip), %rax
    cmp result(%rip), %rax
    jne fail
    mov sentinel+8(%rip), %rax
    cmp result+8(%rip), %rax
    jne fail
    xor %edi, %edi
    jmp done
fail:
    mov $1, %edi
done:
    mov $60, %eax
    syscall
.data
.balign 16
value: .quad 0x400921fb54442d18
extended: .zero 16
sentinel: .quad 0xdeadbeefcafebabe, 0x123456789abcdef0
result: .zero 16
.section .note.GNU-stack,"",@progbits
