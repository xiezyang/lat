.global _start
.text
_start:
    mov $10000, %ecx
1:
    mov $1, %eax
    cmp $2, %eax
    jb 2f
    jmp fail
2:
    /* The successor reads the same carry consumed by the fused jump. */
    setb %dl
    mov $0, %eax
    adc $0, %eax
    cmp $1, %eax
    jne fail
    cmp $1, %dl
    jne fail
    mov $0, %eax
    test %eax, %eax
    jne fail
    /* Fallthrough must retain TEST's zero flag. */
    setz %dl
    cmp $1, %dl
    jne fail
    mov $3, %eax
    cmp $2, %eax
    ja 3f
    jmp fail
3:
    /* Both flags are dead before the next observation. */
    xor %eax, %eax
    test %eax, %eax
    jz 4f
    jmp fail
4:
    sub $1, %ecx
    jne 1b
    xor %edi, %edi
    jmp done
fail:
    mov $1, %edi
done:
    mov $60, %eax
    syscall
.section .note.GNU-stack,"",@progbits
