/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VCVTT_SET_MXCSR control
    mov dword ptr [rip + vcvtt_mxcsr_control], \control
    ldmxcsr dword ptr [rip + vcvtt_mxcsr_control]
    .endm

    .macro VCVTT_SET_FLAGS
    push 0xad7
    popfq
    .endm

    .macro VCVTT_SAVE_RESULT dest, source
    mov qword ptr [rip + vcvtt_result_temp], \dest
    pushfq
    pop r11
    mov qword ptr [rip + vcvtt_rflags_temp], r11
    stmxcsr dword ptr [rip + vcvtt_mxcsr_after]
    mov r11, qword ptr [rip + vcvtt_output_ptr]
    mov r10, qword ptr [rip + vcvtt_result_temp]
    mov qword ptr [r11], r10
    mov r10d, dword ptr [rip + vcvtt_mxcsr_after]
    mov dword ptr [r11 + 8], r10d
    mov dword ptr [r11 + 12], 0
    mov r10, qword ptr [rip + vcvtt_rflags_temp]
    mov qword ptr [r11 + 16], r10
    movq qword ptr [r11 + 24], \source
    add r11, 32
    mov qword ptr [rip + vcvtt_output_ptr], r11
    .endm

    .macro VCVTT64_CASE dest, source, value, control
    VCVTT_SET_MXCSR \control
    movdqu \source, xmmword ptr [rip + \value]
    mov \dest, 0x1122334455667788
    VCVTT_SET_FLAGS
    vcvttsd2si \dest, \source
    VCVTT_SAVE_RESULT \dest, \source
    .endm

    .macro VCVTT32_CASE dest64, dest32, source, value, control
    VCVTT_SET_MXCSR \control
    movdqu \source, xmmword ptr [rip + \value]
    mov \dest64, 0x1122334455667788
    VCVTT_SET_FLAGS
    vcvttsd2si \dest32, \source
    VCVTT_SAVE_RESULT \dest64, \source
    .endm

    .macro VCVTT64_RSP_CASE source, value, control
    VCVTT_SET_MXCSR \control
    movdqu \source, xmmword ptr [rip + \value]
    mov qword ptr [rip + vcvtt_saved_rsp], rsp
    VCVTT_SET_FLAGS
    vcvttsd2si rsp, \source
    mov qword ptr [rip + vcvtt_result_temp], rsp
    mov rsp, qword ptr [rip + vcvtt_saved_rsp]
    mov r10, qword ptr [rip + vcvtt_result_temp]
    VCVTT_SAVE_RESULT r10, \source
    .endm

    .macro VCVTT_RUN_VALUES64
    VCVTT64_CASE rax, xmm0, value_pzero, 0x1f80
    VCVTT64_CASE rax, xmm0, value_nzero, 0x1f80
    VCVTT64_CASE rax, xmm0, value_pone, 0x1f80
    VCVTT64_CASE rax, xmm0, value_none, 0x1f80
    VCVTT64_CASE rax, xmm0, value_onehalf, 0x1f80
    VCVTT64_CASE rax, xmm0, value_nonehalf, 0x1f80
    VCVTT64_CASE rax, xmm0, value_half, 0x1f80
    VCVTT64_CASE rax, xmm0, value_min_subnormal, 0x1f80
    VCVTT64_CASE rax, xmm0, value_nmin_subnormal, 0x1f80
    VCVTT64_CASE rax, xmm0, value_min_subnormal, 0x1fc0
    VCVTT64_CASE rax, xmm0, value_nmin_subnormal, 0x1fc0
    VCVTT64_CASE rax, xmm0, value_min_normal, 0x1f80
    VCVTT64_CASE rax, xmm0, value_qnan, 0x1f80
    VCVTT64_CASE rax, xmm0, value_snan, 0x1f80
    VCVTT64_CASE rax, xmm0, value_pinf, 0x1f80
    VCVTT64_CASE rax, xmm0, value_ninf, 0x1f80
    VCVTT64_CASE rax, xmm0, value_min_i64, 0x1f80
    VCVTT64_CASE rax, xmm0, value_below_min_i64, 0x1f80
    VCVTT64_CASE rax, xmm0, value_above_min_i64, 0x1f80
    VCVTT64_CASE rax, xmm0, value_max_i64_double, 0x1f80
    VCVTT64_CASE rax, xmm0, value_pos_i64_limit, 0x1f80
    VCVTT64_CASE rax, xmm0, value_onehalf, 0x3f80
    VCVTT64_CASE rax, xmm0, value_onehalf, 0x5f80
    VCVTT64_CASE rax, xmm0, value_onehalf, 0x7f80
    VCVTT64_CASE rax, xmm0, value_onehalf, 0x9f80
    VCVTT64_CASE rax, xmm0, value_pone, 0x1fbf
    VCVTT64_CASE r15, xmm15, value_pone_high_snan, 0x1f80
    .endm

    .macro VCVTT_RUN_VALUES32
    VCVTT32_CASE rax, eax, xmm0, value_pzero, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_onehalf, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_nonehalf, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_min_subnormal, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_min_subnormal, 0x1fc0
    VCVTT32_CASE rax, eax, xmm0, value_qnan, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_pinf, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_min_i32, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_below_min_i32, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_above_min_i32, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_max_i32_double, 0x1f80
    VCVTT32_CASE rax, eax, xmm0, value_pos_i32_limit, 0x1f80
    VCVTT32_CASE r15, r15d, xmm15, value_pone_high_snan, 0x1f80
    .endm

    .macro VCVTT_RUN_ALL_REGS
    VCVTT64_CASE rax, xmm0, value_pone, 0x1f80
    VCVTT64_CASE rcx, xmm1, value_none, 0x1f80
    VCVTT64_CASE rdx, xmm2, value_onehalf, 0x1f80
    VCVTT64_CASE rbx, xmm3, value_nonehalf, 0x1f80
    VCVTT64_RSP_CASE xmm4, value_pone, 0x1f80
    VCVTT64_CASE rbp, xmm5, value_none, 0x1f80
    VCVTT64_CASE rsi, xmm6, value_onehalf, 0x1f80
    VCVTT64_CASE rdi, xmm7, value_nonehalf, 0x1f80
    VCVTT64_CASE r8, xmm8, value_pone, 0x1f80
    VCVTT64_CASE r9, xmm9, value_none, 0x1f80
    VCVTT64_CASE r10, xmm10, value_onehalf, 0x1f80
    VCVTT64_CASE r11, xmm11, value_nonehalf, 0x1f80
    VCVTT64_CASE r12, xmm12, value_pone, 0x1f80
    VCVTT64_CASE r13, xmm13, value_none, 0x1f80
    VCVTT64_CASE r14, xmm14, value_onehalf, 0x1f80
    VCVTT64_CASE r15, xmm15, value_nonehalf, 0x1f80
    .endm

    .macro VCVTT_MEMORY_CASE dest64, dest, source, address_variant
    VCVTT_SET_MXCSR 0x1f80
    mov r10, qword ptr [rip + vcvtt_memory_base]
    .if \address_variant == 0
    lea rax, [r10]
    .elseif \address_variant == 1
    lea rax, [r10 + 8192]
    mov r12, 3
    lea rax, [rax + r12 * 2 + 7]
    .elseif \address_variant == 2
    lea rax, [r10 + 16384 + 120]
    .elseif \address_variant == 3
    lea rax, [r10 + 25600]
    .elseif \address_variant == 4
    lea rax, [rip + latx_avx_vcvttsd2si_rip_source]
    .elseif \address_variant == 5
    lea r12, [r10 + 32768 + 1]
    mov rax, r12
    .elseif \address_variant == 6
    lea r13, [r10 + 40960 + 4088]
    mov rax, r13
    .elseif \address_variant == 7
    lea rax, [r10 + 49152 + 4092]
    .elseif \address_variant == 8
    lea rax, [r10 + 57344 + 17]
    .else
    lea rax, [r10 + 61440 + 4088]
    .endif
    mov qword ptr [rip + vcvtt_memory_address], rax
    VCVTT_SET_FLAGS
    vcvttsd2si \dest, qword ptr [rax]
    mov r10, qword ptr [rip + vcvtt_memory_address]
    pxor \source, \source
    movq \source, qword ptr [r10]
    VCVTT_SAVE_RESULT \dest64, \source
    .endm

    .globl latx_avx_single_vcvttsd2si_run
    .type latx_avx_single_vcvttsd2si_run, @function
latx_avx_single_vcvttsd2si_run:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov qword ptr [rip + vcvtt_output_ptr], rdi
    mov qword ptr [rip + vcvtt_output_start], rdi
    mov qword ptr [rip + vcvtt_memory_base], rsi

    VCVTT_RUN_VALUES64
    VCVTT_RUN_VALUES32
    VCVTT_RUN_ALL_REGS

    VCVTT_MEMORY_CASE rax, rax, xmm0, 0
    VCVTT_MEMORY_CASE rax, eax, xmm0, 1
    VCVTT_MEMORY_CASE r15, r15, xmm15, 2
    VCVTT_MEMORY_CASE r15, r15d, xmm15, 3
    VCVTT_MEMORY_CASE rax, rax, xmm0, 4
    VCVTT_MEMORY_CASE r12, r12, xmm12, 5
    VCVTT_MEMORY_CASE r13, r13, xmm13, 6
    VCVTT_MEMORY_CASE rax, eax, xmm0, 7
    VCVTT_MEMORY_CASE rax, rax, xmm0, 8
    VCVTT_MEMORY_CASE r15, r15, xmm15, 9

    VCVTT_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + value_onehalf]
    mov rax, 0x1122334455667788
    VCVTT_SET_FLAGS
    .byte 0xc5, 0xfb, 0x2c, 0xc0
    VCVTT_SAVE_RESULT rax, xmm0

    VCVTT_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + value_nonehalf]
    mov rax, 0x1122334455667788
    VCVTT_SET_FLAGS
    .byte 0xc4, 0xe1, 0x7b, 0x2c, 0xc0
    VCVTT_SAVE_RESULT rax, xmm0

    VCVTT_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + value_onehalf]
    mov rax, 0x1122334455667788
    VCVTT_SET_FLAGS
    .byte 0xc4, 0xe1, 0xfb, 0x2c, 0xc0
    VCVTT_SAVE_RESULT rax, xmm0

    mov rax, qword ptr [rip + vcvtt_output_ptr]
    sub rax, qword ptr [rip + vcvtt_output_start]
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

    .globl latx_avx_single_vcvttsd2si_fault
    .type latx_avx_single_vcvttsd2si_fault, @function
latx_avx_single_vcvttsd2si_fault:
    VCVTT_SET_MXCSR 0x1f80
    mov r15, 0x1122334455667788
    VCVTT_SET_FLAGS
    vcvttsd2si r15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vcvttsd2si_unmasked
    .type latx_avx_single_vcvttsd2si_unmasked, @function
latx_avx_single_vcvttsd2si_unmasked:
    mov qword ptr [rip + vcvtt_output_ptr], rdi
    mov r15, 0x1122334455667788
    cmp esi, 1
    jne .Lvcvtt_precision
    VCVTT_SET_MXCSR 0x1f00
    movdqu xmm15, xmmword ptr [rip + value_qnan]
    jmp .Lvcvtt_ready
.Lvcvtt_precision:
    cmp esi, 2
    jne .Lvcvtt_subnormal_precision
    VCVTT_SET_MXCSR 0x0f80
    movdqu xmm15, xmmword ptr [rip + value_onehalf]
    jmp .Lvcvtt_ready
.Lvcvtt_subnormal_precision:
    cmp esi, 3
    jne .Lvcvtt_no_signal_daz
    VCVTT_SET_MXCSR 0x0f80
    movdqu xmm15, xmmword ptr [rip + value_min_subnormal]
    jmp .Lvcvtt_ready
.Lvcvtt_no_signal_daz:
    cmp esi, 4
    jne .Lvcvtt_no_signal_old_ie
    VCVTT_SET_MXCSR 0x0fc0
    movdqu xmm15, xmmword ptr [rip + value_min_subnormal]
    jmp .Lvcvtt_ready
.Lvcvtt_no_signal_old_ie:
    cmp esi, 5
    jne .Lvcvtt_no_signal_old_pe
    VCVTT_SET_MXCSR 0x1f01
    movdqu xmm15, xmmword ptr [rip + value_pone]
    jmp .Lvcvtt_ready
.Lvcvtt_no_signal_old_pe:
    VCVTT_SET_MXCSR 0x0fa0
    movdqu xmm15, xmmword ptr [rip + value_pone]
.Lvcvtt_ready:
    VCVTT_SET_FLAGS

    .globl latx_avx_single_vcvttsd2si_unmasked_site
latx_avx_single_vcvttsd2si_unmasked_site:
    vcvttsd2si r15, xmm15
    VCVTT_SAVE_RESULT r15, xmm15
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 16
value_pzero:
    .quad 0x0000000000000000, 0x13579bdf2468ace0
value_nzero:
    .quad 0x8000000000000000, 0x13579bdf2468ace0
value_pone:
    .quad 0x3ff0000000000000, 0x13579bdf2468ace0
value_none:
    .quad 0xbff0000000000000, 0x13579bdf2468ace0
value_half:
    .quad 0x3fe0000000000000, 0x13579bdf2468ace0
value_onehalf:
    .quad 0x3ff8000000000000, 0x13579bdf2468ace0
value_nonehalf:
    .quad 0xbff8000000000000, 0x13579bdf2468ace0
value_min_normal:
    .quad 0x0010000000000000, 0x13579bdf2468ace0
value_min_subnormal:
    .quad 0x0000000000000001, 0x13579bdf2468ace0
value_nmin_subnormal:
    .quad 0x8000000000000001, 0x13579bdf2468ace0
value_qnan:
    .quad 0x7ff8000000001234, 0x13579bdf2468ace0
value_snan:
    .quad 0x7ff0000000001234, 0x13579bdf2468ace0
value_pinf:
    .quad 0x7ff0000000000000, 0x13579bdf2468ace0
value_ninf:
    .quad 0xfff0000000000000, 0x13579bdf2468ace0
value_min_i32:
    .quad 0xc1e0000000000000, 0
value_below_min_i32:
    .quad 0xc1e0000000000001, 0
value_above_min_i32:
    .quad 0xc1dfffffffffffff, 0
value_max_i32_double:
    .quad 0x41dfffffffffffff, 0
value_pos_i32_limit:
    .quad 0x41e0000000000000, 0
value_min_i64:
    .quad 0xc3e0000000000000, 0
value_below_min_i64:
    .quad 0xc3e0000000000001, 0
value_above_min_i64:
    .quad 0xc3dfffffffffffff, 0
value_max_i64_double:
    .quad 0x43dfffffffffffff, 0
value_pos_i64_limit:
    .quad 0x43e0000000000000, 0
value_pone_high_snan:
    .quad 0x3ff0000000000000, 0x7ff0000000009876

    .section .data
    .balign 16
    .globl latx_avx_vcvttsd2si_rip_source
latx_avx_vcvttsd2si_rip_source:
    .quad 0xfff0000000000000

    .balign 8
vcvtt_output_ptr:
    .quad 0
vcvtt_output_start:
    .quad 0
vcvtt_memory_base:
    .quad 0
vcvtt_memory_address:
    .quad 0
vcvtt_result_temp:
    .quad 0
vcvtt_rflags_temp:
    .quad 0
vcvtt_saved_rsp:
    .quad 0
vcvtt_mxcsr_control:
    .long 0x1f80
vcvtt_mxcsr_after:
    .long 0

    .section .note.GNU-stack,"",@progbits
