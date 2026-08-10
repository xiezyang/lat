/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro INIT_VECTORS
    vmovdqu ymm0, ymmword ptr [rip + vector_seed + 0]
    vmovdqu ymm1, ymmword ptr [rip + vector_seed + 32]
    vmovdqu ymm2, ymmword ptr [rip + vector_seed + 64]
    vmovdqu ymm3, ymmword ptr [rip + vector_seed + 96]
    vmovdqu ymm4, ymmword ptr [rip + vector_seed + 128]
    vmovdqu ymm5, ymmword ptr [rip + vector_seed + 160]
    vmovdqu ymm6, ymmword ptr [rip + vector_seed + 192]
    vmovdqu ymm7, ymmword ptr [rip + vector_seed + 224]
    vmovdqu ymm8, ymmword ptr [rip + vector_seed + 256]
    vmovdqu ymm9, ymmword ptr [rip + vector_seed + 288]
    vmovdqu ymm10, ymmword ptr [rip + vector_seed + 320]
    vmovdqu ymm11, ymmword ptr [rip + vector_seed + 352]
    vmovdqu ymm12, ymmword ptr [rip + vector_seed + 384]
    vmovdqu ymm13, ymmword ptr [rip + vector_seed + 416]
    vmovdqu ymm14, ymmword ptr [rip + vector_seed + 448]
    vmovdqu ymm15, ymmword ptr [rip + vector_seed + 480]
    .endm

    .macro SAVE_VECTORS offset
    vmovdqu ymmword ptr [r11 + \offset + 0], ymm0
    vmovdqu ymmword ptr [r11 + \offset + 32], ymm1
    vmovdqu ymmword ptr [r11 + \offset + 64], ymm2
    vmovdqu ymmword ptr [r11 + \offset + 96], ymm3
    vmovdqu ymmword ptr [r11 + \offset + 128], ymm4
    vmovdqu ymmword ptr [r11 + \offset + 160], ymm5
    vmovdqu ymmword ptr [r11 + \offset + 192], ymm6
    vmovdqu ymmword ptr [r11 + \offset + 224], ymm7
    vmovdqu ymmword ptr [r11 + \offset + 256], ymm8
    vmovdqu ymmword ptr [r11 + \offset + 288], ymm9
    vmovdqu ymmword ptr [r11 + \offset + 320], ymm10
    vmovdqu ymmword ptr [r11 + \offset + 352], ymm11
    vmovdqu ymmword ptr [r11 + \offset + 384], ymm12
    vmovdqu ymmword ptr [r11 + \offset + 416], ymm13
    vmovdqu ymmword ptr [r11 + \offset + 448], ymm14
    vmovdqu ymmword ptr [r11 + \offset + 480], ymm15
    .endm

    .macro SET_GPRS
    mov rax, 0x1111111111111111
    mov rbx, 0x2222222222222222
    mov rcx, 0x3333333333333333
    mov rdx, 0x4444444444444444
    mov rsi, 0x5555555555555555
    mov rdi, 0x6666666666666666
    mov rbp, 0x7777777777777777
    mov r8,  0x8888888888888888
    mov r9,  0x9999999999999999
    mov r10, 0xaaaaaaaaaaaaaaaa
    mov r11, 0xbbbbbbbbbbbbbbbb
    mov r12, 0xcccccccccccccccc
    mov r13, 0xdddddddddddddddd
    mov r14, 0xeeeeeeeeeeeeeeee
    mov r15, 0xffffffffffffffff
    .endm

    .macro SAVE_GPRS local_offset
    mov qword ptr [rsp + \local_offset + 0], rax
    mov qword ptr [rsp + \local_offset + 8], rbx
    mov qword ptr [rsp + \local_offset + 16], rcx
    mov qword ptr [rsp + \local_offset + 24], rdx
    mov qword ptr [rsp + \local_offset + 32], rsi
    mov qword ptr [rsp + \local_offset + 40], rdi
    mov qword ptr [rsp + \local_offset + 48], rbp
    mov qword ptr [rsp + \local_offset + 56], rsp
    mov qword ptr [rsp + \local_offset + 64], r8
    mov qword ptr [rsp + \local_offset + 72], r9
    mov qword ptr [rsp + \local_offset + 80], r10
    mov qword ptr [rsp + \local_offset + 88], r11
    mov qword ptr [rsp + \local_offset + 96], r12
    mov qword ptr [rsp + \local_offset + 104], r13
    mov qword ptr [rsp + \local_offset + 112], r14
    mov qword ptr [rsp + \local_offset + 120], r15
    .endm

    .macro COPY_GPRS local_offset, output_offset
    mov rax, qword ptr [rsp + \local_offset + 0]
    mov qword ptr [r11 + \output_offset + 0], rax
    mov rax, qword ptr [rsp + \local_offset + 8]
    mov qword ptr [r11 + \output_offset + 8], rax
    mov rax, qword ptr [rsp + \local_offset + 16]
    mov qword ptr [r11 + \output_offset + 16], rax
    mov rax, qword ptr [rsp + \local_offset + 24]
    mov qword ptr [r11 + \output_offset + 24], rax
    mov rax, qword ptr [rsp + \local_offset + 32]
    mov qword ptr [r11 + \output_offset + 32], rax
    mov rax, qword ptr [rsp + \local_offset + 40]
    mov qword ptr [r11 + \output_offset + 40], rax
    mov rax, qword ptr [rsp + \local_offset + 48]
    mov qword ptr [r11 + \output_offset + 48], rax
    mov rax, qword ptr [rsp + \local_offset + 56]
    mov qword ptr [r11 + \output_offset + 56], rax
    mov rax, qword ptr [rsp + \local_offset + 64]
    mov qword ptr [r11 + \output_offset + 64], rax
    mov rax, qword ptr [rsp + \local_offset + 72]
    mov qword ptr [r11 + \output_offset + 72], rax
    mov rax, qword ptr [rsp + \local_offset + 80]
    mov qword ptr [r11 + \output_offset + 80], rax
    mov rax, qword ptr [rsp + \local_offset + 88]
    mov qword ptr [r11 + \output_offset + 88], rax
    mov rax, qword ptr [rsp + \local_offset + 96]
    mov qword ptr [r11 + \output_offset + 96], rax
    mov rax, qword ptr [rsp + \local_offset + 104]
    mov qword ptr [r11 + \output_offset + 104], rax
    mov rax, qword ptr [rsp + \local_offset + 112]
    mov qword ptr [r11 + \output_offset + 112], rax
    mov rax, qword ptr [rsp + \local_offset + 120]
    mov qword ptr [r11 + \output_offset + 120], rax
    .endm

    .macro SET_FLAGS_AND_MXCSR local_offset
    mov eax, 7
    cmp eax, 13
    pushfq
    pop rax
    mov qword ptr [rsp + \local_offset + 0], rax
    ldmxcsr dword ptr [rip + mxcsr_seed]
    stmxcsr dword ptr [rsp + \local_offset + 8]
    .endm

    .macro SAVE_FLAGS_AND_MXCSR local_offset
    pushfq
    pop rax
    mov qword ptr [rsp + \local_offset + 16], rax
    stmxcsr dword ptr [rsp + \local_offset + 24]
    .endm

    .globl latx_avx_single_vzeroall_run
    .type latx_avx_single_vzeroall_run, @function
latx_avx_single_vzeroall_run:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 4096
    mov qword ptr [rsp + 0], rdi

    INIT_VECTORS
    mov r11, qword ptr [rsp + 0]
    SAVE_VECTORS 0

    INIT_VECTORS
    SET_FLAGS_AND_MXCSR 1600
    SET_GPRS
    SAVE_GPRS 512
    vzeroall
    SAVE_GPRS 768
    SAVE_FLAGS_AND_MXCSR 1600
    mov r11, qword ptr [rsp + 0]
    SAVE_VECTORS 512

    INIT_VECTORS
    SET_FLAGS_AND_MXCSR 1640
    SET_GPRS
    SAVE_GPRS 1024
    vzeroupper
    SAVE_GPRS 1280
    SAVE_FLAGS_AND_MXCSR 1640
    mov r11, qword ptr [rsp + 0]
    SAVE_VECTORS 1024

    COPY_GPRS 512, 1536
    COPY_GPRS 768, 1664
    COPY_GPRS 1024, 1792
    COPY_GPRS 1280, 1920
    mov rax, qword ptr [rsp + 1600 + 0]
    mov qword ptr [r11 + 2048 + 0], rax
    mov rax, qword ptr [rsp + 1600 + 16]
    mov qword ptr [r11 + 2048 + 8], rax
    mov eax, dword ptr [rsp + 1600 + 8]
    mov dword ptr [r11 + 2048 + 16], eax
    mov eax, dword ptr [rsp + 1600 + 24]
    mov dword ptr [r11 + 2048 + 20], eax
    mov rax, qword ptr [rsp + 1640 + 0]
    mov qword ptr [r11 + 2048 + 24], rax
    mov rax, qword ptr [rsp + 1640 + 16]
    mov qword ptr [r11 + 2048 + 32], rax
    mov eax, dword ptr [rsp + 1640 + 8]
    mov dword ptr [r11 + 2048 + 40], eax
    mov eax, dword ptr [rsp + 1640 + 24]
    mov dword ptr [r11 + 2048 + 44], eax

    add rsp, 4096
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

    .section .rodata
    .balign 32
vector_seed:
    .quad 0x1000000000000000, 0x8000000000000000, 0x7ff8000000000040, 0xffffffffffff0000
    .quad 0x1000000000000001, 0x8000000000000001, 0x7ff8000000000041, 0xffffffffffff0001
    .quad 0x1000000000000002, 0x8000000000000002, 0x7ff8000000000042, 0xffffffffffff0002
    .quad 0x1000000000000003, 0x8000000000000003, 0x7ff8000000000043, 0xffffffffffff0003
    .quad 0x1000000000000004, 0x8000000000000004, 0x7ff8000000000044, 0xffffffffffff0004
    .quad 0x1000000000000005, 0x8000000000000005, 0x7ff8000000000045, 0xffffffffffff0005
    .quad 0x1000000000000006, 0x8000000000000006, 0x7ff8000000000046, 0xffffffffffff0006
    .quad 0x1000000000000007, 0x8000000000000007, 0x7ff8000000000047, 0xffffffffffff0007
    .quad 0x1000000000000008, 0x8000000000000008, 0x7ff8000000000048, 0xffffffffffff0008
    .quad 0x1000000000000009, 0x8000000000000009, 0x7ff8000000000049, 0xffffffffffff0009
    .quad 0x100000000000000a, 0x800000000000000a, 0x7ff800000000004a, 0xffffffffffff000a
    .quad 0x100000000000000b, 0x800000000000000b, 0x7ff800000000004b, 0xffffffffffff000b
    .quad 0x100000000000000c, 0x800000000000000c, 0x7ff800000000004c, 0xffffffffffff000c
    .quad 0x100000000000000d, 0x800000000000000d, 0x7ff800000000004d, 0xffffffffffff000d
    .quad 0x100000000000000e, 0x800000000000000e, 0x7ff800000000004e, 0xffffffffffff000e
    .quad 0x100000000000000f, 0x800000000000000f, 0x7ff800000000004f, 0xffffffffffff000f
mxcsr_seed:
    .long 0x00005f80

    .section .note.GNU-stack,"",@progbits
