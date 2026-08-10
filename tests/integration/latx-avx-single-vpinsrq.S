/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_vpinsrq_run
    .type latx_avx_single_vpinsrq_run, @function
latx_avx_single_vpinsrq_run:
    push r12
    push r13
    push r15
    mov rdx, rsi

    lea rcx, [rip + vpinsrq_vector_random]
    movdqu xmm0, xmmword ptr [rcx]
    mov rax, qword ptr [rip + vpinsrq_scalar_random]
    .set vpinsrq_imm, 0
    .rept 256
        vpinsrq xmm0, xmm0, rax, vpinsrq_imm
        movdqu xmmword ptr [rdi + vpinsrq_imm * 16], xmm0
        movdqu xmm0, xmmword ptr [rcx]
        .set vpinsrq_imm, vpinsrq_imm + 1
    .endr

    lea rcx, [rip + vpinsrq_vector_random]
    movdqu xmm15, xmmword ptr [rcx]
    .set vpinsrq_mem_imm, 0
    .rept 256
        vpinsrq xmm15, xmm15, qword ptr [rdx + 13], vpinsrq_mem_imm
        movdqu xmmword ptr [rdi + 4096 + vpinsrq_mem_imm * 16], xmm15
        movdqu xmm15, xmmword ptr [rcx]
        .set vpinsrq_mem_imm, vpinsrq_mem_imm + 1
    .endr

    lea rcx, [rip + vpinsrq_vector_zero]
    movdqu xmm0, xmmword ptr [rcx]
    mov rax, qword ptr [rip + vpinsrq_scalar_zero]
    vpinsrq xmm0, xmm0, rax, 0
    movdqu xmmword ptr [rdi + 8192], xmm0

    lea rcx, [rip + vpinsrq_vector_ones]
    movdqu xmm0, xmmword ptr [rcx]
    mov rax, qword ptr [rip + vpinsrq_scalar_ones]
    vpinsrq xmm0, xmm0, rax, 1
    movdqu xmmword ptr [rdi + 8208], xmm0

    lea rcx, [rip + vpinsrq_vector_high]
    movdqu xmm0, xmmword ptr [rcx]
    mov r15, qword ptr [rip + vpinsrq_scalar_high]
    vpinsrq xmm15, xmm0, r15, 0
    movdqu xmmword ptr [rdi + 8224], xmm15

    lea rcx, [rip + vpinsrq_vector_incrementing]
    movdqu xmm15, xmmword ptr [rcx]
    mov r15, qword ptr [rip + vpinsrq_scalar_incrementing]
    vpinsrq xmm15, xmm15, r15, 1
    movdqu xmmword ptr [rdi + 8240], xmm15

    lea rcx, [rip + vpinsrq_vector_55aa]
    movdqu xmm15, xmmword ptr [rcx]
    mov rax, qword ptr [rip + vpinsrq_scalar_55aa]
    vpinsrq xmm7, xmm15, rax, 254
    movdqu xmmword ptr [rdi + 8256], xmm7

    lea rcx, [rip + vpinsrq_vector_random]
    movdqu xmm7, xmmword ptr [rcx]
    mov r15, qword ptr [rip + vpinsrq_scalar_random]
    vpinsrq xmm15, xmm7, r15, 255
    movdqu xmmword ptr [rdi + 8272], xmm15

    lea rcx, [rip + vpinsrq_vector_zero]
    movdqu xmm0, xmmword ptr [rcx]
    vpinsrq xmm0, xmm0, qword ptr [rdx], 0
    movdqu xmmword ptr [rdi + 8288], xmm0

    lea rcx, [rip + vpinsrq_vector_ones]
    movdqu xmm0, xmmword ptr [rcx]
    vpinsrq xmm15, xmm0, qword ptr [rdx + 1], 1
    movdqu xmmword ptr [rdi + 8304], xmm15

    lea rcx, [rip + vpinsrq_vector_high]
    movdqu xmm15, xmmword ptr [rcx]
    mov r12, rdx
    mov r13, 3
    vpinsrq xmm7, xmm15, qword ptr [r12 + r13 * 2 + 7], 254
    movdqu xmmword ptr [rdi + 8320], xmm7

    lea rcx, [rip + vpinsrq_vector_incrementing]
    movdqu xmm15, xmmword ptr [rcx]
    vpinsrq xmm15, xmm15, qword ptr [rdx + 4088], 255
    movdqu xmmword ptr [rdi + 8336], xmm15

    lea rcx, [rip + vpinsrq_vector_55aa]
    movdqu xmm15, xmmword ptr [rcx]
    vpinsrq xmm0, xmm15, qword ptr [rip + vpinsrq_scalar_high], 0
    movdqu xmmword ptr [rdi + 8352], xmm0

    lea rcx, [rip + vpinsrq_vector_random]
    movdqu xmm0, xmmword ptr [rcx]
    vpinsrq xmm15, xmm0, qword ptr [rip + vpinsrq_scalar_55aa], 1
    movdqu xmmword ptr [rdi + 8368], xmm15

    pop r15
    pop r13
    pop r12
    ret

    .globl latx_avx_single_vpinsrq_trace
    .type latx_avx_single_vpinsrq_trace, @function
latx_avx_single_vpinsrq_trace:
    push r15
    lea rax, [rip + vpinsrq_vector_random]
    movdqu xmm15, xmmword ptr [rax]
    mov r15, qword ptr [rip + vpinsrq_scalar_high]
    mov ecx, 2
.Lvpinsrq_trace_loop:
    vpinsrq xmm15, xmm15, r15, 1
    dec ecx
    jne .Lvpinsrq_trace_loop
    pop r15
    ret

    .globl latx_avx_single_vpinsrq_fault_invalid
    .type latx_avx_single_vpinsrq_fault_invalid, @function
latx_avx_single_vpinsrq_fault_invalid:
    lea rax, [rip + vpinsrq_vector_random]
    movdqu xmm15, xmmword ptr [rax]
    mov eax, 1
    vpinsrq xmm15, xmm15, qword ptr [rax], 0
    ret

    .globl latx_avx_single_vpinsrq_fault_cross_page
    .type latx_avx_single_vpinsrq_fault_cross_page, @function
latx_avx_single_vpinsrq_fault_cross_page:
    lea rax, [rip + vpinsrq_vector_random]
    movdqu xmm15, xmmword ptr [rax]
    lea rax, [rdi + 4092]
    vpinsrq xmm15, xmm15, qword ptr [rax], 1
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 16
vpinsrq_vector_zero:
    .quad 0, 0
vpinsrq_vector_ones:
    .quad -1, -1
vpinsrq_vector_high:
    .quad 0x8000000000000000, 0x0000000000000001
vpinsrq_vector_incrementing:
    .byte 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    .byte 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
vpinsrq_vector_55aa:
    .quad 0x55aa55aa55aa55aa, 0xaa55aa55aa55aa55
vpinsrq_vector_random:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850

vpinsrq_scalar_zero:
    .quad 0
vpinsrq_scalar_ones:
    .quad -1
vpinsrq_scalar_high:
    .quad 0x8000000000000000
vpinsrq_scalar_incrementing:
    .quad 0x0706050403020100
vpinsrq_scalar_55aa:
    .quad 0x55aa55aa55aa55aa
vpinsrq_scalar_random:
    .quad 0xd3e9a16c7204bf58

    .section .note.GNU-stack,"",@progbits
