/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix

    .data
    .balign 16
    .globl latx_avx_vmovd_rip_input
latx_avx_vmovd_rip_input:
    .long 0
    .fill 12, 1, 0xa5
    .balign 16
    .globl latx_avx_vmovd_rip_store
latx_avx_vmovd_rip_store:
    .fill 16, 1, 0x5a
    .balign 16
    .globl latx_avx_vmovd_xmm_seed
latx_avx_vmovd_xmm_seed:
    .long 0
    .long 0x77665544
    .quad 0xffeeddccbbaa9988
    .balign 8
latx_avx_vmovd_saved_rsp:
    .quad 0

    .text

    .macro VMOVD_GPR_TO_XMM xmm, reg32, offset
    pcmpeqd \xmm, \xmm
    mov \reg32, edi
    vmovd \xmm, \reg32
    movdqu xmmword ptr [rip + latx_avx_vmovd_register_results + \offset], \xmm
    .endm

    .macro VMOVD_XMM_TO_GPR reg32, reg64, xmm, offset
    movdqu \xmm, xmmword ptr [rip + latx_avx_vmovd_xmm_seed]
    mov \reg64, -1
    vmovd \reg32, \xmm
    mov qword ptr [rip + latx_avx_vmovd_register_results + \offset], \reg64
    .endm

    .globl latx_avx_single_vmovd_registers
    .type latx_avx_single_vmovd_registers, @function
latx_avx_single_vmovd_registers:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    VMOVD_GPR_TO_XMM xmm0, eax, 0
    VMOVD_GPR_TO_XMM xmm1, ecx, 16
    VMOVD_GPR_TO_XMM xmm2, edx, 32
    VMOVD_GPR_TO_XMM xmm3, ebx, 48

    pcmpeqd xmm4, xmm4
    mov qword ptr [rip + latx_avx_vmovd_saved_rsp], rsp
    mov esp, edi
    vmovd xmm4, esp
    mov rsp, qword ptr [rip + latx_avx_vmovd_saved_rsp]
    movdqu xmmword ptr [rip + latx_avx_vmovd_register_results + 64], xmm4

    VMOVD_GPR_TO_XMM xmm5, ebp, 80
    VMOVD_GPR_TO_XMM xmm6, esi, 96
    VMOVD_GPR_TO_XMM xmm7, edi, 112
    VMOVD_GPR_TO_XMM xmm8, r8d, 128
    VMOVD_GPR_TO_XMM xmm9, r9d, 144
    VMOVD_GPR_TO_XMM xmm10, r10d, 160
    VMOVD_GPR_TO_XMM xmm11, r11d, 176
    VMOVD_GPR_TO_XMM xmm12, r12d, 192
    VMOVD_GPR_TO_XMM xmm13, r13d, 208
    VMOVD_GPR_TO_XMM xmm14, r14d, 224
    VMOVD_GPR_TO_XMM xmm15, r15d, 240

    VMOVD_XMM_TO_GPR eax, rax, xmm0, 256
    VMOVD_XMM_TO_GPR ecx, rcx, xmm1, 272
    VMOVD_XMM_TO_GPR edx, rdx, xmm2, 288
    VMOVD_XMM_TO_GPR ebx, rbx, xmm3, 304

    movdqu xmm4, xmmword ptr [rip + latx_avx_vmovd_xmm_seed]
    mov qword ptr [rip + latx_avx_vmovd_saved_rsp], rsp
    vmovd esp, xmm4
    mov rax, rsp
    mov rsp, qword ptr [rip + latx_avx_vmovd_saved_rsp]
    mov qword ptr [rip + latx_avx_vmovd_register_results + 320], rax

    VMOVD_XMM_TO_GPR ebp, rbp, xmm5, 336
    VMOVD_XMM_TO_GPR esi, rsi, xmm6, 352
    VMOVD_XMM_TO_GPR edi, rdi, xmm7, 368
    VMOVD_XMM_TO_GPR r8d, r8, xmm8, 384
    VMOVD_XMM_TO_GPR r9d, r9, xmm9, 400
    VMOVD_XMM_TO_GPR r10d, r10, xmm10, 416
    VMOVD_XMM_TO_GPR r11d, r11, xmm11, 432
    VMOVD_XMM_TO_GPR r12d, r12, xmm12, 448
    VMOVD_XMM_TO_GPR r13d, r13, xmm13, 464
    VMOVD_XMM_TO_GPR r14d, r14, xmm14, 480
    VMOVD_XMM_TO_GPR r15d, r15, xmm15, 496

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

    .globl latx_avx_single_vmovd_memory
    .type latx_avx_single_vmovd_memory, @function
latx_avx_single_vmovd_memory:
    push r15
    mov r10, rcx
    mov r11, r8

    pcmpeqd xmm0, xmm0
    vmovd xmm0, dword ptr [rsi]
    movdqu xmmword ptr [r10 + 0], xmm0

    pcmpeqd xmm15, xmm15
    mov eax, 4
    vmovd xmm15, dword ptr [rsi + rax * 4 + 8]
    movdqu xmmword ptr [r10 + 16], xmm15

    pcmpeqd xmm1, xmm1
    vmovd xmm1, dword ptr [rsi + 120]
    movdqu xmmword ptr [r10 + 32], xmm1

    pcmpeqd xmm14, xmm14
    vmovd xmm14, dword ptr [rsi + 4096]
    movdqu xmmword ptr [r10 + 48], xmm14

    pcmpeqd xmm2, xmm2
    vmovd xmm2, dword ptr [rip + latx_avx_vmovd_rip_input]
    movdqu xmmword ptr [r10 + 64], xmm2

    pcmpeqd xmm13, xmm13
    lea rax, [rsi + 257]
    vmovd xmm13, dword ptr [rax]
    movdqu xmmword ptr [r10 + 80], xmm13

    pcmpeqd xmm3, xmm3
    vmovd xmm3, dword ptr [rdx]
    movdqu xmmword ptr [r10 + 96], xmm3

    sub rsp, 128
    mov dword ptr [rsp + 0x64], edi
    pcmpeqd xmm1, xmm1
    pushfq
    pop r15
    mov rax, r15
    xor rax, 0x8d5
    push rax
    popfq
    pushfq
    pop qword ptr [r10 + 128]
    stmxcsr dword ptr [r10 + 136]
    /* ToDesk+0xdb731d: vmovd xmm1,dword ptr [rsp+0x64]. */
    .byte 0xc5, 0xf9, 0x6e, 0x4c, 0x24, 0x64
    pushfq
    pop qword ptr [r10 + 144]
    stmxcsr dword ptr [r10 + 152]
    push r15
    popfq
    movdqu xmmword ptr [r10 + 112], xmm1
    add rsp, 128

    movd xmm0, edi
    movd xmm15, edi
    vmovd dword ptr [r11], xmm0
    mov eax, 4
    vmovd dword ptr [r11 + rax * 4 + 48], xmm15
    vmovd dword ptr [r11 + 120], xmm0
    vmovd dword ptr [r11 + 4096], xmm15
    lea rax, [r11 + 257]
    vmovd dword ptr [rax], xmm0
    vmovd dword ptr [rip + latx_avx_vmovd_rip_store], xmm15
    vmovd dword ptr [r9], xmm0

    pop r15
    ret

    .globl latx_avx_single_vmovd_fault_load
    .type latx_avx_single_vmovd_fault_load, @function
latx_avx_single_vmovd_fault_load:
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vmovd_xrstor_area]
    movdqu xmm0, xmmword ptr [rip + vmovd_fault_initial]
    vmovd xmm0, dword ptr [rdi]
    ret

    .globl latx_avx_single_vmovd_fault_store
    .type latx_avx_single_vmovd_fault_store, @function
latx_avx_single_vmovd_fault_store:
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vmovd_xrstor_area]
    movd xmm0, esi
    vmovd dword ptr [rdi], xmm0
    ret

    .globl latx_avx_single_vmovd_rt_sigreturn
    .type latx_avx_single_vmovd_rt_sigreturn, @function
latx_avx_single_vmovd_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .data
    .balign 16
vmovd_fault_initial:
    .fill 16, 1, 0xa7

    .balign 64
vmovd_xrstor_area:
    .short 0x037f
    .short 0
    .zero 20
    .long 0x00001f80
    .long 0x0000ffff
    .zero 128
    .zero 256
    .zero 96
    .quad 6
    .quad 0
    .zero 48
    .quad 0x1122334455667788, 0x99aabbccddeeff00

    .section .note.GNU-stack,"",@progbits
