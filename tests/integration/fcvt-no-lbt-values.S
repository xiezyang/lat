/* Native-reference records: extended value, binary64 result, x87 status. */
.global _start
.text
_start:
    lea inputs(%rip), %rbx
    mov $12, %ebp
next:
    fninit
    fldl (%rbx)
    fstpt record(%rip)
    fldt record(%rip)
    fstpl record+16(%rip)
    fnstsw record+24(%rip)
    mov $1, %eax
    mov $1, %edi
    lea record(%rip), %rsi
    mov $32, %edx
    syscall
    add $8, %rbx
    dec %ebp
    jnz next
    mov $60, %eax
    xor %edi, %edi
    syscall
.data
.balign 16
inputs:
    .quad 0, 0x8000000000000000
    .quad 0x3ff0000000000000, 0xbff0000000000000
    .quad 1, 0x000fffffffffffff
    .quad 0x0010000000000000, 0x7fefffffffffffff
    .quad 0x7ff0000000000000, 0xfff0000000000000
    .quad 0x7ff8000000000123, 0x7ff0000000000123
.balign 16
record: .zero 32
.section .note.GNU-stack,"",@progbits
