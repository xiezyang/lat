/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .globl latx_avx_single_vptest_run
    .type latx_avx_single_vptest_run, @function
latx_avx_single_vptest_run:
    vmovdqu ymm0, ymmword ptr [rip + data_zero]
    vmovdqu ymm1, ymmword ptr [rip + data_zero]
    vptest xmm0, xmm1
    jmp .Lsave0
    .p2align 4
.Lsave0:
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_ones]
    vmovdqu ymm1, ymmword ptr [rip + data_ones]
    vptest xmm0, xmm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_zero]
    vmovdqu ymm1, ymmword ptr [rip + data_ones]
    vptest xmm0, xmm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_bit]
    vmovdqu ymm1, ymmword ptr [rip + data_mixed]
    vptest xmm0, xmm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_zero]
    vmovdqu ymm1, ymmword ptr [rip + data_zero]
    vptest ymm0, ymm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_bit]
    vptest ymm0, ymm0
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm0
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_bit]
    vmovdqu ymm1, ymmword ptr [rip + data_mixed]
    vptest ymm0, ymm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_ones]
    vptest ymm0, ymmword ptr [rip + data_misaligned+1]
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    ret

    .globl latx_avx_single_vptest_fault128
latx_avx_single_vptest_fault128:
    vmovdqu ymm0, ymmword ptr [rip + data_ones]
    vptest xmm0, xmmword ptr [rdi]
    ret
    .globl latx_avx_single_vptest_fault256
latx_avx_single_vptest_fault256:
    vmovdqu ymm0, ymmword ptr [rip + data_ones]
    vptest ymm0, ymmword ptr [rdi]
    ret

    .section .rodata
    .balign 32
data_zero:
    .quad 0,0,0,0
data_ones:
    .quad -1,-1,-1,-1
data_bit:
    .quad 1,1,1,1
data_mixed:
    .quad 3,3,3,3
data_misaligned:
    .byte 0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55
    .byte 0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa
    .byte 0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55
    .byte 0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa
    .section .note.GNU-stack,"",@progbits
