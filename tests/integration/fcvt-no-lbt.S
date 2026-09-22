/* Static x86-64 smoke test for binary64 <-> x87 extended conversion. */
.global _start
.text
_start:
    fninit
    fldl value(%rip)
    fstpt extended(%rip)
    fldt extended(%rip)
    fstpl result(%rip)
    mov $1, %eax
    mov $1, %edi
    lea value(%rip), %rsi
    mov $32, %edx
    syscall
    mov value(%rip), %rax
    cmp result(%rip), %rax
    jne fail
    xor %edi, %edi
    jmp exit
fail:
    mov $1, %edi
exit:
    mov $60, %eax
    syscall
.data
.balign 16
value: .quad 0x400921fb54442d18
extended: .zero 16
result: .quad 0
.section .note.GNU-stack,"",@progbits
