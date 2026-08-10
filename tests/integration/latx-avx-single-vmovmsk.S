/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_vmovmsk_run
latx_avx_single_vmovmsk_run:
    xor r11d, r11d
.Lps_xmm:
    vmovdqu xmm0, xmmword ptr [rsi]
    mov rax, -1
    vmovmskps eax, xmm0
    mov qword ptr [rdi], rax
    mov eax, r11d
    or eax, 0x0000
    mov qword ptr [rdi + 8], rax
    add rdi, 16
    vmovdqu xmm15, xmmword ptr [rsi]
    mov rax, -1
    vmovmskps eax, xmm15
    mov qword ptr [rdi], rax
    mov eax, r11d
    or eax, 0x1000
    mov qword ptr [rdi + 8], rax
    add rdi, 16
    add rsi, 16
    inc r11d
    cmp r11d, 16
    jb .Lps_xmm

    xor r11d, r11d
.Lps_ymm:
    vmovdqu ymm0, ymmword ptr [rdx]
    mov r9, -1
    vmovmskps r9d, ymm0
    mov qword ptr [rdi], r9
    mov eax, r11d
    or eax, 0x10000
    mov qword ptr [rdi + 8], rax
    add rdi, 16
    vmovdqu ymm15, ymmword ptr [rdx]
    mov r9, -1
    vmovmskps r9d, ymm15
    mov qword ptr [rdi], r9
    mov eax, r11d
    or eax, 0x11000
    mov qword ptr [rdi + 8], rax
    add rdi, 16
    add rdx, 32
    inc r11d
    cmp r11d, 256
    jb .Lps_ymm

    xor r11d, r11d
.Lpd_xmm:
    vmovdqu xmm0, xmmword ptr [rcx]
    mov rax, -1
    vmovmskpd eax, xmm0
    mov qword ptr [rdi], rax
    mov eax, r11d
    or eax, 0x20000
    mov qword ptr [rdi + 8], rax
    add rdi, 16
    vmovdqu xmm15, xmmword ptr [rcx]
    mov rax, -1
    vmovmskpd eax, xmm15
    mov qword ptr [rdi], rax
    mov eax, r11d
    or eax, 0x21000
    mov qword ptr [rdi + 8], rax
    add rdi, 16
    add rcx, 16
    inc r11d
    cmp r11d, 4
    jb .Lpd_xmm

    xor r11d, r11d
.Lpd_ymm:
    vmovdqu ymm0, ymmword ptr [r8]
    mov r9, -1
    vmovmskpd r9d, ymm0
    mov qword ptr [rdi], r9
    mov eax, r11d
    or eax, 0x30000
    mov qword ptr [rdi + 8], rax
    add rdi, 16
    vmovdqu ymm15, ymmword ptr [r8]
    mov r9, -1
    vmovmskpd r9d, ymm15
    mov qword ptr [rdi], r9
    mov eax, r11d
    or eax, 0x31000
    mov qword ptr [rdi + 8], rax
    add rdi, 16
    add r8, 32
    inc r11d
    cmp r11d, 16
    jb .Lpd_ymm

    vzeroupper
    ret

    .section .note.GNU-stack,"",@progbits
