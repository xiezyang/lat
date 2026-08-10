/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VCVTSD2SS_RESTORE_YMM
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vcvtsd2ss_xrstor_area]
    .endm

    .macro VCVTSD2SS_SET_MXCSR control
    mov dword ptr [rip + vcvtsd2ss_mxcsr_control], \control
    ldmxcsr dword ptr [rip + vcvtsd2ss_mxcsr_control]
    .endm

    .macro VCVTSD2SS_SAVE target
    stmxcsr dword ptr [rip + vcvtsd2ss_mxcsr_after]
    pushfq
    pop r11
    call latx_avx_single_vcvtsd2ss_observe_marker
    test rdi, rdi
    jz .Lvcvtsd2ss_skip_save_\@
    .if \target == 0
    movdqu xmm12, xmm0
    .else
    movdqu xmm12, xmm15
    .endif
    movdqu xmmword ptr [rdi], xmm12
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vcvtsd2ss_xsave_area]
    movdqu xmm12, xmmword ptr [rip + vcvtsd2ss_xsave_area + 576 + \target * 16]
    movdqu xmmword ptr [rdi + 16], xmm12
    mov eax, dword ptr [rip + vcvtsd2ss_mxcsr_after]
    mov dword ptr [rdi + 32], eax
    mov dword ptr [rdi + 36], 0
    mov qword ptr [rdi + 40], r11
    mov qword ptr [rdi + 48], 0
    mov qword ptr [rdi + 56], 0
    add rdi, 64
.Lvcvtsd2ss_skip_save_\@:
    .endm

    .macro VCVTSD2SS_REG target, value, control
    VCVTSD2SS_RESTORE_YMM
    VCVTSD2SS_SET_MXCSR \control
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vcvtsd2ss_src1_seed]
    movdqu xmm1, xmmword ptr [rip + \value]
    vcvtsd2ss xmm0, xmm0, xmm1
    .else
    movdqu xmm15, xmmword ptr [rip + vcvtsd2ss_src1_seed]
    movdqu xmm14, xmmword ptr [rip + \value]
    vcvtsd2ss xmm15, xmm15, xmm14
    .endif
    VCVTSD2SS_SAVE \target
    .endm

    .macro VCVTSD2SS_MEM target, offset, control
    VCVTSD2SS_RESTORE_YMM
    VCVTSD2SS_SET_MXCSR \control
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vcvtsd2ss_src1_seed]
    lea rbx, [rsi + \offset]
    vcvtsd2ss xmm0, xmm0, qword ptr [rbx]
    .else
    movdqu xmm15, xmmword ptr [rip + vcvtsd2ss_src1_seed]
    lea rbx, [rsi + \offset]
    vcvtsd2ss xmm15, xmm15, qword ptr [rbx]
    .endif
    VCVTSD2SS_SAVE \target
    .endm

    .globl latx_avx_single_vcvtsd2ss_run
    .type latx_avx_single_vcvtsd2ss_run, @function
latx_avx_single_vcvtsd2ss_run:
    push rbx
    push r12
    push r15
    mov qword ptr [rip + vcvtsd2ss_output_ptr], rdi
    mov qword ptr [rip + vcvtsd2ss_output_start], rdi

    VCVTSD2SS_REG 0, vcvtsd2ss_one, 0x1f80
    VCVTSD2SS_REG 0, vcvtsd2ss_three, 0x1f80
    VCVTSD2SS_REG 0, vcvtsd2ss_three, 0x3f80
    VCVTSD2SS_REG 0, vcvtsd2ss_three, 0x5f80
    VCVTSD2SS_REG 0, vcvtsd2ss_one_third, 0x7f80
    VCVTSD2SS_REG 15, vcvtsd2ss_min_normal, 0x1f80
    VCVTSD2SS_REG 15, vcvtsd2ss_min_subnormal, 0x1f80
    VCVTSD2SS_REG 15, vcvtsd2ss_max_normal, 0x1f80
    VCVTSD2SS_REG 0, vcvtsd2ss_qnan, 0x1f80
    VCVTSD2SS_REG 15, vcvtsd2ss_snan, 0x1f80
    VCVTSD2SS_REG 0, vcvtsd2ss_inf, 0x1f80
    VCVTSD2SS_REG 15, vcvtsd2ss_negative, 0x1f80

    VCVTSD2SS_MEM 0, 1, 0x1f80
    VCVTSD2SS_MEM 0, 4090, 0x1f80
    VCVTSD2SS_MEM 15, 8193, 0x1f80
    VCVTSD2SS_MEM 15, 12287, 0x3f80
    VCVTSD2SS_MEM 0, 16385, 0x5f80
    VCVTSD2SS_MEM 15, 20479, 0x7f80

    mov rax, qword ptr [rip + vcvtsd2ss_output_ptr]
    sub rax, qword ptr [rip + vcvtsd2ss_output_start]
    pop r15
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vcvtsd2ss_unmasked
    .type latx_avx_single_vcvtsd2ss_unmasked, @function
latx_avx_single_vcvtsd2ss_unmasked:
    VCVTSD2SS_RESTORE_YMM
    cmp esi, 1
    je .Lvcvtsd2ss_unmasked_invalid
    cmp esi, 2
    je .Lvcvtsd2ss_unmasked_precision
    cmp esi, 3
    je .Lvcvtsd2ss_unmasked_overflow
    cmp esi, 4
    je .Lvcvtsd2ss_unmasked_underflow
.Lvcvtsd2ss_unmasked_precision:
    VCVTSD2SS_SET_MXCSR 0x0f80
    movdqu xmm14, xmmword ptr [rip + vcvtsd2ss_src1_seed]
    movdqu xmm13, xmmword ptr [rip + vcvtsd2ss_one_third]
    jmp latx_avx_single_vcvtsd2ss_unmasked_site
.Lvcvtsd2ss_unmasked_invalid:
    VCVTSD2SS_SET_MXCSR 0x1f00
    movdqu xmm14, xmmword ptr [rip + vcvtsd2ss_src1_seed]
    movdqu xmm13, xmmword ptr [rip + vcvtsd2ss_snan]
    jmp latx_avx_single_vcvtsd2ss_unmasked_site
.Lvcvtsd2ss_unmasked_overflow:
    VCVTSD2SS_SET_MXCSR 0x1b80
    movdqu xmm14, xmmword ptr [rip + vcvtsd2ss_src1_seed]
    movdqu xmm13, xmmword ptr [rip + vcvtsd2ss_max_normal]
    jmp latx_avx_single_vcvtsd2ss_unmasked_site
.Lvcvtsd2ss_unmasked_underflow:
    VCVTSD2SS_SET_MXCSR 0x1780
    movdqu xmm14, xmmword ptr [rip + vcvtsd2ss_src1_seed]
    movdqu xmm13, xmmword ptr [rip + vcvtsd2ss_min_subnormal]

    .globl latx_avx_single_vcvtsd2ss_unmasked_site
latx_avx_single_vcvtsd2ss_unmasked_site:
    vcvtsd2ss xmm15, xmm14, xmm13
    ret

    .globl latx_avx_single_vcvtsd2ss_observe_marker
latx_avx_single_vcvtsd2ss_observe_marker:
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 16
vcvtsd2ss_src1_seed: .long 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd
vcvtsd2ss_one: .quad 0x3ff0000000000000, 0
vcvtsd2ss_three: .quad 0x4008000000000000, 0
vcvtsd2ss_one_third: .quad 0x3fd5555555555555, 0
vcvtsd2ss_min_normal: .quad 0x0010000000000000, 0
vcvtsd2ss_min_subnormal: .quad 1, 0
vcvtsd2ss_max_normal: .quad 0x7fefffffffffffff, 0
vcvtsd2ss_qnan: .quad 0x7ff8000000001234, 0
vcvtsd2ss_snan: .quad 0x7ff0000000001234, 0
vcvtsd2ss_inf: .quad 0x7ff0000000000000, 0
vcvtsd2ss_negative: .quad 0xc008000000000000, 0

    .section .data
    .balign 64
vcvtsd2ss_xrstor_area:
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
    .zero 224
    .quad 0x1122334455667788, 0x99aabbccddeeff00

    .section .bss
    .balign 64
vcvtsd2ss_xsave_area: .zero 4096
vcvtsd2ss_output_ptr: .zero 8
vcvtsd2ss_output_start: .zero 8
vcvtsd2ss_mxcsr_control: .zero 4
vcvtsd2ss_mxcsr_after: .zero 4
    .section .note.GNU-stack,"",@progbits
