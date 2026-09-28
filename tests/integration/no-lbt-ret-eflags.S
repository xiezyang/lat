.equ __NR_rt_sigaction, 13
.equ __NR_sigaltstack, 131
.equ __NR_mmap, 9
.equ __NR_exit, 60

.equ SIGSEGV, 11
.equ SA_SIGINFO, 4
.equ SA_ONSTACK, 0x08000000
.equ SA_RESTORER, 0x04000000
.equ PROT_NONE, 0
.equ PROT_READ_WRITE, 3
.equ MAP_PRIVATE_ANONYMOUS, 0x22
.equ EFLAGS_MASK, 0x8d5
.equ EFLAGS_NO_AF_MASK, 0x8c5
.equ UCONTEXT_RSP, 160
.equ UCONTEXT_RIP, 168
.equ UCONTEXT_EFLAGS, 176

.section .bss
.balign 16
.Lsigaction:
    .zero 32
.Laltstack:
    .zero 24
.Lfault_stack:
    .zero 8

.section .text
.global _start
.type _start, @function
_start:
    /* Test a shared callee from two distinct call sites. CALL and RET retain
     * the flags produced by the callee, even though its target is dynamic. */
    mov $1, %rdi
    mov $2, %rsi
    call .Lcmp_ret
    pushfq
    pop %rax
    and $EFLAGS_MASK, %eax
    cmp $0x95, %eax
    jne .Lfail

    mov $2, %rdi
    mov $1, %rsi
    call .Lcmp_ret
    pushfq
    pop %rax
    and $EFLAGS_MASK, %eax
    cmp $0, %eax
    jne .Lfail

    xor %edi, %edi
    xor %esi, %esi
    call .Ltest_ret
    pushfq
    pop %rax
    and $EFLAGS_NO_AF_MASK, %eax
    cmp $0x44, %eax
    jne .Lfail

    mov $1, %edi
    mov $2, %esi
    call .Ltest_ret
    pushfq
    pop %rax
    and $EFLAGS_NO_AF_MASK, %eax
    cmp $0x44, %eax
    jne .Lfail

    /* Install an alternate signal stack so the handler runs even when RET
     * faults with RSP in a PROT_NONE page. */
    mov $__NR_mmap, %eax
    xor %edi, %edi
    mov $8192, %esi
    mov $PROT_READ_WRITE, %edx
    mov $MAP_PRIVATE_ANONYMOUS, %r10d
    mov $-1, %r8
    xor %r9d, %r9d
    syscall
    test %rax, %rax
    js .Lfail
    mov %rax, .Laltstack(%rip)
    movq $0, .Laltstack+8(%rip)
    movq $8192, .Laltstack+16(%rip)
    mov $__NR_sigaltstack, %eax
    lea .Laltstack(%rip), %rdi
    xor %esi, %esi
    syscall
    test %rax, %rax
    js .Lfail

    lea .Lsegv_handler(%rip), %rax
    mov %rax, .Lsigaction(%rip)
    movq $(SA_SIGINFO | SA_ONSTACK | SA_RESTORER), .Lsigaction+8(%rip)
    lea .Lrestorer(%rip), %rax
    mov %rax, .Lsigaction+16(%rip)
    movq $0, .Lsigaction+24(%rip)
    mov $__NR_rt_sigaction, %eax
    mov $SIGSEGV, %edi
    lea .Lsigaction(%rip), %rsi
    xor %edx, %edx
    mov $8, %r10d
    syscall
    test %rax, %rax
    js .Lfail

    /* RET reads this separately mapped PROT_NONE page. The handler checks
     * that CMP's flags were already present in the fault context. */
    mov $__NR_mmap, %eax
    xor %edi, %edi
    mov $4096, %esi
    mov $PROT_NONE, %edx
    mov $MAP_PRIVATE_ANONYMOUS, %r10d
    mov $-1, %r8
    xor %r9d, %r9d
    syscall
    test %rax, %rax
    js .Lfail
    mov %rax, .Lfault_stack(%rip)
    mov %rax, %rsp
    mov $1, %rdi
    mov $2, %rsi
    jmp .Lcmp_ret

.Lcmp_ret:
    cmp %rsi, %rdi
.Lcmp_ret_insn:
    ret

.Ltest_ret:
    test %rsi, %rdi
    ret

.Lsegv_handler:
    cmp $SIGSEGV, %edi
    jne .Lhandler_fail
    mov UCONTEXT_RIP(%rdx), %rax
    lea .Lcmp_ret_insn(%rip), %rcx
    cmp %rcx, %rax
    jne .Lhandler_fail
    mov UCONTEXT_RSP(%rdx), %rax
    cmp .Lfault_stack(%rip), %rax
    jne .Lhandler_fail
    mov UCONTEXT_EFLAGS(%rdx), %rax
    and $EFLAGS_MASK, %eax
    cmp $0x95, %eax
    jne .Lhandler_fail
    xor %edi, %edi
    jmp .Lexit

.Lhandler_fail:
    mov $1, %edi
    jmp .Lexit

.Lrestorer:
    mov $__NR_exit, %eax
    mov $2, %edi
    syscall

.Lfail:
    mov $1, %edi
.Lexit:
    mov $__NR_exit, %eax
    syscall
.size _start, .-_start
