.equ __NR_exit, 60

.section .data
.balign 16
source:
    .byte 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
    .byte 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    .byte 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27
    .byte 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f

.balign 16
destination:
    .zero 64

.balign 65536
cross_page_source_base:
    .zero 65533
cross_page_source:
    .byte 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37
    .byte 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f

.balign 65536
cross_page_destination_base:
    .zero 65531
cross_page_destination:
    .zero 16

.section .text
.global _start
.type _start, @function
_start:
    lea source(%rip), %rsi
    lea destination(%rip), %rdi

    /* Cover all 256 source/destination alignment combinations, including
     * the new scalar-doubleword path at offset 8 and byte fallback at 1..7.
     */
    xor %r8d, %r8d
.Lsource_offset:
    xor %r9d, %r9d
.Ldestination_offset:
    xor %ecx, %ecx
.Lfill:
    movb $0xa5, (%rdi,%rcx)
    inc %ecx
    cmp $64, %ecx
    jne .Lfill

    lea (%rsi,%r8), %r10
    lea 16(%rdi,%r9), %r11
    movdqu (%r10), %xmm0
    movdqu %xmm0, (%r11)
    xor %ecx, %ecx
.Lverify:
    movzbl (%r10,%rcx), %eax
    cmpb %al, (%r11,%rcx)
    jne .Lfail
    inc %ecx
    cmp $16, %ecx
    jne .Lverify

    /* Check the entire destination outside the requested 16-byte range. */
    xor %ecx, %ecx
.Lcanary:
    lea (%rdi,%rcx), %rdx
    cmp %r11, %rdx
    jb .Lcheck_canary
    lea 16(%r11), %rax
    cmp %rax, %rdx
    jb .Lcanary_next
.Lcheck_canary:
    cmpb $0xa5, (%rdx)
    jne .Lfail
.Lcanary_next:
    inc %ecx
    cmp $64, %ecx
    jne .Lcanary
    inc %r9d
    cmp $16, %r9d
    jne .Ldestination_offset
    inc %r8d
    cmp $16, %r8d
    jne .Lsource_offset

    lea cross_page_source(%rip), %rsi
    lea cross_page_destination(%rip), %rdi
    movdqu (%rsi), %xmm1
    movdqu %xmm1, (%rdi)

    xor %ecx, %ecx
.Lverify_cross_page:
    movzbl (%rsi,%rcx), %eax
    cmpb %al, (%rdi,%rcx)
    jne .Lfail
    inc %ecx
    cmp $16, %ecx
    jne .Lverify_cross_page

    xor %edi, %edi
    jmp .Lexit

.Lfail:
    mov $1, %edi

.Lexit:
    mov $__NR_exit, %eax
    syscall
.size _start, .-_start
