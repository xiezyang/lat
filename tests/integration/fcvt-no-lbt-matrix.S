/* Emit deterministic binary results for comparison with native x86-64. */
.macro record
    fstpl (%rbx)
#ifndef FCVT_CONTIGUOUS
    mov $1, %eax
    mov $1, %edi
    mov %rbx, %rsi
    mov $8, %edx
    syscall
#endif
    add $8, %rbx
    fninit
.endm
.macro binary insn:vararg
    fldl two(%rip)
    fldl eight(%rip)
    \insn
    record
.endm
.macro memory insn:vararg
    fldl eight(%rip)
    \insn
    record
.endm
.global _start
.text
_start:
    lea results(%rip), %rbx
    fninit
    binary fadd %st(1), %st
    binary faddp %st, %st(1)
    binary fsub %st(1), %st
    binary fsubp %st, %st(1)
    binary fsubr %st(1), %st
    binary fsubrp %st, %st(1)
    binary fmul %st(1), %st
    binary fmulp %st, %st(1)
    binary fdiv %st(1), %st
    binary fdivp %st, %st(1)
    binary fdivr %st(1), %st
    binary fdivrp %st, %st(1)
    memory faddl two(%rip)
    memory fsubl two(%rip)
    memory fsubrl two(%rip)
    memory fmull two(%rip)
    memory fdivl two(%rip)
    memory fdivrl two(%rip)
    memory fiaddl integer(%rip)
    memory fisubl integer(%rip)
    memory fisubrl integer(%rip)
    memory fimull integer(%rip)
    memory fidivl integer(%rip)
    memory fidivrl integer(%rip)
    fldl eight(%rip)
    fsqrt
    record
    fldl fraction(%rip)
    frndint
    record
    binary fscale
    filds integer(%rip)
    record
    fildl integer(%rip)
    record
    fildq integer(%rip)
    record
    flds single(%rip)
    record
    fldl eight(%rip)
    fstl (%rbx)
    add $8, %rbx
    record
    fldl eight(%rip)
    fistl (%rbx)
    add $8, %rbx
    fistpl (%rbx)
    add $8, %rbx
    fldl eight(%rip)
    fistpq (%rbx)
    add $8, %rbx
    fldl eight(%rip)
    fistps (%rbx)
    add $8, %rbx
    /* All four x87 rounding modes. */
    lea controls(%rip), %rbp
    mov $4, %r15d
round_loop:
    fldcw (%rbp)
    fldl fraction(%rip)
    fistpl (%rbx)
    add $8, %rbx
    add $2, %rbp
    dec %r15d
    jnz round_loop
    fninit
    mov $1, %eax
    mov $1, %edi
    lea results(%rip), %rsi
    mov %rbx, %rdx
    sub %rsi, %rdx
    syscall
    mov $60, %eax
    xor %edi, %edi
    syscall
.data
.balign 16
two: .double 2.0
eight: .double 8.0
fraction: .double 2.75
single: .float 1.25
.balign 8
integer: .quad 2
controls: .short 0x37f, 0x77f, 0xb7f, 0xf7f
.bss
.balign 16
results: .zero 1024
.section .note.GNU-stack,"",@progbits
