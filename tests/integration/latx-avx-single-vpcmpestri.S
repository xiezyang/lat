/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .globl latx_avx_single_vpcmpestri_run
    .type latx_avx_single_vpcmpestri_run, @function
latx_avx_single_vpcmpestri_run:
    movdqu xmm0, xmmword ptr [rip + data_a]
    movdqu xmm1, xmmword ptr [rip + data_b]
    mov eax, 16
    mov edx, 16
    vpcmpestri xmm0, xmm1, 0x00
    pushfq
    pop r11
    mov qword ptr [rdi], rcx
    mov qword ptr [rdi+8], r11
    mov dword ptr [rdi+16], eax
    mov dword ptr [rdi+20], edx
    add rdi, 64
    movdqu xmm0, xmmword ptr [rip + data_a]
    movdqu xmm1, xmmword ptr [rip + data_b]
    mov eax, 0
    mov edx, 0
    vpcmpestri xmm0, xmm1, 0x05
    pushfq
    pop r11
    mov qword ptr [rdi], rcx
    mov qword ptr [rdi+8], r11
    mov dword ptr [rdi+16], eax
    mov dword ptr [rdi+20], edx
    add rdi, 64
    movdqu xmm0, xmmword ptr [rip + data_a]
    movdqu xmm1, xmmword ptr [rip + data_b]
    mov eax, 8
    mov edx, 8
    vpcmpestri xmm0, xmm1, 0x0a
    pushfq
    pop r11
    mov qword ptr [rdi], rcx
    mov qword ptr [rdi+8], r11
    mov dword ptr [rdi+16], eax
    mov dword ptr [rdi+20], edx
    add rdi, 64
    movdqu xmm0, xmmword ptr [rip + data_a]
    movdqu xmm1, xmmword ptr [rip + data_b]
    mov eax, 16
    mov edx, 8
    vpcmpestri xmm0, xmm1, 0x1f
    pushfq
    pop r11
    mov qword ptr [rdi], rcx
    mov qword ptr [rdi+8], r11
    mov dword ptr [rdi+16], eax
    mov dword ptr [rdi+20], edx
    add rdi, 64
    movdqu xmm0, xmmword ptr [rip + data_a]
    mov eax, 16
    mov edx, 16
    vpcmpestri xmm0, xmm0, 0x2d
    pushfq
    pop r11
    mov qword ptr [rdi], rcx
    mov qword ptr [rdi+8], r11
    mov dword ptr [rdi+16], eax
    mov dword ptr [rdi+20], edx
    add rdi, 64
    movdqu xmm0, xmmword ptr [rip + data_a]
    mov eax, 16
    mov edx, 16
    vpcmpestri xmm0, xmmword ptr [rip + data_b], 0x37
    pushfq
    pop r11
    mov qword ptr [rdi], rcx
    mov qword ptr [rdi+8], r11
    mov dword ptr [rdi+16], eax
    mov dword ptr [rdi+20], edx
    add rdi, 64
    movdqu xmm2, xmmword ptr [rip + data_a]
    movdqu xmm1, xmmword ptr [rip + data_b]
    mov eax, 15
    mov edx, 1
    vpcmpestri xmm2, xmm1, 0x4b
    pushfq
    pop r11
    mov qword ptr [rdi], rcx
    mov qword ptr [rdi+8], r11
    mov dword ptr [rdi+16], eax
    mov dword ptr [rdi+20], edx
    add rdi, 64
    movdqu xmm0, xmmword ptr [rip + data_b]
    mov eax, 1
    mov edx, 15
    vpcmpestri xmm0, xmmword ptr [rip + data_a], 0x5f
    pushfq
    pop r11
    mov qword ptr [rdi], rcx
    mov qword ptr [rdi+8], r11
    mov dword ptr [rdi+16], eax
    mov dword ptr [rdi+20], edx
    add rdi, 64
    ret

    .globl latx_avx_single_vpcmpestri_fault
    .type latx_avx_single_vpcmpestri_fault, @function
latx_avx_single_vpcmpestri_fault:
    movdqu xmm0, xmmword ptr [rip + data_a]
    mov eax, 16
    mov edx, 16
    vpcmpestri xmm0, xmmword ptr [rdi], 0x00
    ret

    .section .rodata
    .balign 16
data_a:
    .byte 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07
    .byte 0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
data_b:
    .byte 0x00,0x80,0x7f,0xff,0x01,0x02,0x03,0x04
    .byte 0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c

    .section .note.GNU-stack,"",@progbits
