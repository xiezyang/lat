/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VCVTSI_RESTORE_YMM
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vcvtsi_xrstor_area]
    .endm

    .macro VCVTSI_SET_MXCSR control
    mov dword ptr [rip + vcvtsi_mxcsr_control], \control
    ldmxcsr dword ptr [rip + vcvtsi_mxcsr_control]
    .endm

    .macro VCVTSI_SET_FLAGS
    push 0x246
    popfq
    .endm

    .macro VCVTSI_RECORD target
    stmxcsr dword ptr [rip + vcvtsi_mxcsr_after]
    pushfq
    pop r11
    mov edx, 0
    call latx_avx_single_vcvtsi2sd_observe_marker
    test rdi, rdi
    jz .Lvcvtsi_skip_record_\@
    .if \target == 0
    movdqu xmm12, xmm0
    .else
    movdqu xmm12, xmm15
    .endif
    movdqu xmmword ptr [rdi], xmm12
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vcvtsi_xsave_area]
    .if \target == 0
    movdqu xmm12, xmmword ptr [rip + vcvtsi_xsave_area + 576]
    .else
    movdqu xmm12, xmmword ptr [rip + vcvtsi_xsave_area + 816]
    .endif
    movdqu xmmword ptr [rdi + 16], xmm12
    mov eax, dword ptr [rip + vcvtsi_mxcsr_after]
    mov dword ptr [rdi + 32], eax
    mov dword ptr [rdi + 36], 0
    mov qword ptr [rdi + 40], r11
    mov qword ptr [rdi + 48], 0
    mov qword ptr [rdi + 56], 0
    add rdi, 64
.Lvcvtsi_skip_record_\@:
    .endm

    .macro VCVTSI_REG32 target, src, gpr, value, control, target_no
    VCVTSI_SET_MXCSR \control
    movdqu \src, xmmword ptr [rip + vcvtsi_src_seed]
    mov \gpr, dword ptr [rip + \value]
    VCVTSI_SET_FLAGS
    vcvtsi2sd \target, \src, \gpr
    VCVTSI_RECORD \target_no
    .endm

    .macro VCVTSI_REG64 target, src, gpr, value, control, target_no
    VCVTSI_SET_MXCSR \control
    movdqu \src, xmmword ptr [rip + vcvtsi_src_seed_alt]
    mov \gpr, qword ptr [rip + \value]
    VCVTSI_SET_FLAGS
    vcvtsi2sd \target, \src, \gpr
    VCVTSI_RECORD \target_no
    .endm

    .macro VCVTSI_SPECIAL target, src, gpr, value, upper, target_no
    VCVTSI_SET_MXCSR 0x1f80
    movdqu \src, xmmword ptr [rip + \upper]
    mov \gpr, qword ptr [rip + \value]
    VCVTSI_SET_FLAGS
    vcvtsi2sd \target, \src, \gpr
    VCVTSI_RECORD \target_no
    .endm

    .macro VCVTSI_ADDRESS variant, width
    .if \variant == 0
    lea rbx, [rsi]
    .elseif \variant == 1
    lea rbx, [rsi + 8192]
    mov r12, 3
    lea rbx, [rbx + r12 * 2 + 7]
    .elseif \variant == 2
    lea rbx, [rsi + 16384]
    .elseif \variant == 3
    lea rbx, [rsi + 20480]
    .elseif \variant == 4
    lea rbx, [rip + latx_avx_vcvtsi2sd_rip_source]
    .elseif \variant == 5
    lea rbx, [rsi + 32768]
    .elseif \variant == 6
    lea rbx, [rsi]
    .else
    .if \width == 32
    lea rbx, [rsi + 8192]
    .else
    lea rbx, [rsi + 12288]
    .endif
    .endif
    .endm

    .macro VCVTSI_MEM32 variant, target, src, target_no
    VCVTSI_SET_MXCSR 0x1f80
    movdqu \src, xmmword ptr [rip + vcvtsi_src_seed]
    VCVTSI_ADDRESS \variant, 32
    VCVTSI_SET_FLAGS
    .if \variant == 2
    vcvtsi2sd \target, \src, dword ptr [rbx + 120]
    .elseif \variant == 3
    vcvtsi2sd \target, \src, dword ptr [rbx + 4096]
    .elseif \variant == 5
    vcvtsi2sd \target, \src, dword ptr [rbx + 1]
    .elseif \variant == 6
    vcvtsi2sd \target, \src, dword ptr [rbx + 4092]
    .elseif \variant == 7
    vcvtsi2sd \target, \src, dword ptr [rbx + 4095]
    .else
    vcvtsi2sd \target, \src, dword ptr [rbx]
    .endif
    VCVTSI_RECORD \target_no
    .endm

    .macro VCVTSI_MEM64 variant, target, src, target_no
    VCVTSI_SET_MXCSR 0x1f80
    movdqu \src, xmmword ptr [rip + vcvtsi_src_seed_alt]
    VCVTSI_ADDRESS \variant, 64
    VCVTSI_SET_FLAGS
    .if \variant == 2
    vcvtsi2sd \target, \src, qword ptr [rbx + 120]
    .elseif \variant == 3
    vcvtsi2sd \target, \src, qword ptr [rbx + 4096]
    .elseif \variant == 5
    vcvtsi2sd \target, \src, qword ptr [rbx + 1]
    .elseif \variant == 6
    vcvtsi2sd \target, \src, qword ptr [rbx + 4088]
    .elseif \variant == 7
    vcvtsi2sd \target, \src, qword ptr [rbx + 4092]
    .else
    vcvtsi2sd \target, \src, qword ptr [rbx]
    .endif
    VCVTSI_RECORD \target_no
    .endm

    .globl latx_avx_single_vcvtsi2sd_run
latx_avx_single_vcvtsi2sd_run:
    push rbx
    push r12
    push r15

    VCVTSI_REG32 xmm0, xmm0, eax, vcvtsi_i32_zero, 0x1f80, 0
    VCVTSI_REG32 xmm0, xmm1, r15d, vcvtsi_i32_one, 0x1f80, 0
    VCVTSI_REG32 xmm0, xmm0, eax, vcvtsi_i32_minus_one, 0x1f80, 0
    VCVTSI_REG32 xmm0, xmm1, r15d, vcvtsi_i32_min, 0x1f80, 0
    VCVTSI_REG32 xmm15, xmm15, r15d, vcvtsi_i32_max, 0x1f80, 15
    VCVTSI_REG32 xmm15, xmm14, eax, vcvtsi_i32_min, 0x1f80, 15
    VCVTSI_REG32 xmm15, xmm15, r15d, vcvtsi_i32_one, 0x1f80, 15
    VCVTSI_REG32 xmm15, xmm14, eax, vcvtsi_i32_minus_one, 0x1f80, 15

    VCVTSI_REG64 xmm0, xmm0, rax, vcvtsi_i64_min, 0x1f80, 0
    VCVTSI_REG64 xmm0, xmm1, r15, vcvtsi_i64_max, 0x1f80, 0
    VCVTSI_REG64 xmm0, xmm0, rax, vcvtsi_i64_p53p1, 0x1f80, 0
    VCVTSI_REG64 xmm0, xmm1, r15, vcvtsi_i64_p53p1, 0x3f80, 0
    VCVTSI_REG64 xmm15, xmm15, r15, vcvtsi_i64_p53p1, 0x5f80, 15
    VCVTSI_REG64 xmm15, xmm14, rax, vcvtsi_i64_p53p1, 0x7f80, 15
    VCVTSI_REG64 xmm15, xmm15, r15, vcvtsi_i64_p53m1, 0x1f80, 15
    VCVTSI_REG64 xmm15, xmm14, rax, vcvtsi_i64_minus_one, 0x1f80, 15

    VCVTSI_MEM32 0, xmm0, xmm0, 0
    VCVTSI_MEM32 1, xmm0, xmm1, 0
    VCVTSI_MEM32 2, xmm0, xmm0, 0
    VCVTSI_MEM32 3, xmm0, xmm1, 0
    VCVTSI_MEM32 4, xmm15, xmm15, 15
    VCVTSI_MEM32 5, xmm15, xmm14, 15
    VCVTSI_MEM32 6, xmm15, xmm15, 15
    VCVTSI_MEM32 7, xmm15, xmm14, 15

    VCVTSI_MEM64 0, xmm0, xmm0, 0
    VCVTSI_MEM64 1, xmm0, xmm1, 0
    VCVTSI_MEM64 2, xmm0, xmm0, 0
    VCVTSI_MEM64 3, xmm0, xmm1, 0
    VCVTSI_MEM64 4, xmm15, xmm15, 15
    VCVTSI_MEM64 5, xmm15, xmm14, 15
    VCVTSI_MEM64 6, xmm15, xmm15, 15
    VCVTSI_MEM64 7, xmm15, xmm14, 15

    VCVTSI_SPECIAL xmm0, xmm0, rax, vcvtsi_i64_one, vcvtsi_upper_qnan, 0
    VCVTSI_SPECIAL xmm0, xmm1, r15, vcvtsi_i64_one, vcvtsi_upper_inf, 0
    VCVTSI_SPECIAL xmm15, xmm15, r15, vcvtsi_i64_one, vcvtsi_upper_subnormal, 15
    VCVTSI_SPECIAL xmm15, xmm14, rax, vcvtsi_i64_one, vcvtsi_upper_negzero, 15

    pop r15
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vcvtsi2sd_fault32
latx_avx_single_vcvtsi2sd_fault32:
    VCVTSI_RESTORE_YMM
    VCVTSI_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + vcvtsi_fault_seed]
    VCVTSI_SET_FLAGS
    vcvtsi2sd xmm15, xmm15, dword ptr [rdi]
    ret

    .globl latx_avx_single_vcvtsi2sd_fault64
latx_avx_single_vcvtsi2sd_fault64:
    VCVTSI_RESTORE_YMM
    VCVTSI_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + vcvtsi_fault_seed]
    VCVTSI_SET_FLAGS
    vcvtsi2sd xmm15, xmm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vcvtsi2sd_precision_unmasked
latx_avx_single_vcvtsi2sd_precision_unmasked:
    VCVTSI_RESTORE_YMM
    VCVTSI_SET_MXCSR 0x0f80
    movdqu xmm15, xmmword ptr [rip + vcvtsi_fault_seed]
    mov r15, qword ptr [rip + vcvtsi_i64_p53p1]
    VCVTSI_SET_FLAGS
    vcvtsi2sd xmm15, xmm15, r15
    ret

    .globl latx_avx_single_vcvtsi2sd_fault_observe
latx_avx_single_vcvtsi2sd_fault_observe:
    jmp latx_avx_single_vcvtsi2sd_observe_marker
    .globl latx_avx_single_vcvtsi2sd_observe_marker
latx_avx_single_vcvtsi2sd_observe_marker:
    vcvtsi2sd xmm13, xmm13, edx
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vcvtsi_src_seed: .quad 0x0123456789abcdef, 0xfedcba9876543210
vcvtsi_src_seed_alt: .quad 0x0f1e2d3c4b5a6978, 0x8877665544332211
vcvtsi_fault_seed: .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vcvtsi_i32_zero: .long 0
vcvtsi_i32_one: .long 1
vcvtsi_i32_minus_one: .long -1
vcvtsi_i32_min: .long 0x80000000
vcvtsi_i32_max: .long 0x7fffffff
    .balign 8
vcvtsi_i64_minus_one: .quad -1
vcvtsi_i64_one: .quad 1
vcvtsi_i64_p53m1: .quad 0x001fffffffffffff
vcvtsi_i64_p53p1: .quad 0x0020000000000001
vcvtsi_i64_min: .quad 0x8000000000000000
vcvtsi_i64_max: .quad 0x7fffffffffffffff
vcvtsi_upper_qnan: .quad 0x0123456789abcdef, 0x7ff8000000000001
vcvtsi_upper_inf: .quad 0x0123456789abcdef, 0x7ff0000000000000
vcvtsi_upper_subnormal: .quad 0x0123456789abcdef, 0x0000000000000001
vcvtsi_upper_negzero: .quad 0x0123456789abcdef, 0x8000000000000000

    .section .data
    .balign 8
    .globl latx_avx_vcvtsi2sd_rip_source
latx_avx_vcvtsi2sd_rip_source: .quad 0
    .balign 64
vcvtsi_xrstor_area:
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
    .quad 0x0123456789abcdef, 0xfedcba9876543210

    .section .bss
    .balign 64
vcvtsi_xsave_area: .zero 4096
vcvtsi_mxcsr_control: .zero 4
vcvtsi_mxcsr_after: .zero 4
    .section .note.GNU-stack,"",@progbits
