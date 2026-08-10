/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VUCOMISD_SET_MXCSR control
    mov dword ptr [rip + vucomisd_mxcsr_control], \control
    ldmxcsr dword ptr [rip + vucomisd_mxcsr_control]
    .endm

    .macro VUCOMISD_SET_FLAGS
    push 0xad7
    popfq
    .endm

    .macro VUCOMISD_SAVE_RESULT source
    pushfq
    pop rax
    mov qword ptr [rdi], rax
    stmxcsr dword ptr [rdi + 8]
    mov dword ptr [rdi + 12], 0
    movdqu xmmword ptr [rdi + 16], \source
    add rdi, 32
    .endm

    .macro VUCOMISD_REG_CASE left_reg, right_reg, left, right, control
    VUCOMISD_SET_MXCSR \control
    movdqu \left_reg, xmmword ptr [rip + \left]
    movdqu \right_reg, xmmword ptr [rip + \right]
    VUCOMISD_SET_FLAGS
    vucomisd \left_reg, \right_reg
    VUCOMISD_SAVE_RESULT \left_reg
    .endm

    .macro VUCOMISD_VALUE_CASE left, right, control
    VUCOMISD_REG_CASE xmm0, xmm1, \left, \right, \control
    VUCOMISD_REG_CASE xmm15, xmm14, \left, \right, \control
    .endm

    .macro VUCOMISD_RUN_VALUES
    VUCOMISD_VALUE_CASE value_two, value_pone, 0x1f80
    VUCOMISD_VALUE_CASE value_pone, value_two, 0x1f80
    VUCOMISD_VALUE_CASE value_pone, value_pone, 0x1f80
    VUCOMISD_VALUE_CASE value_pzero, value_nzero, 0x1f80
    VUCOMISD_VALUE_CASE value_ntwo, value_none, 0x1f80
    VUCOMISD_VALUE_CASE value_none, value_ntwo, 0x1f80
    VUCOMISD_VALUE_CASE value_pinf, value_max_normal, 0x1f80
    VUCOMISD_VALUE_CASE value_ninf, value_nmax_normal, 0x1f80
    VUCOMISD_VALUE_CASE value_qnan, value_pone, 0x1f80
    VUCOMISD_VALUE_CASE value_pone, value_qnan_alt, 0x1f80
    VUCOMISD_VALUE_CASE value_snan, value_pone, 0x1f80
    VUCOMISD_VALUE_CASE value_pone, value_snan_alt, 0x1f80
    VUCOMISD_VALUE_CASE value_qnan, value_qnan_alt, 0x1f80
    VUCOMISD_VALUE_CASE value_min_subnormal, value_pzero, 0x1f80
    VUCOMISD_VALUE_CASE value_nmin_subnormal, value_nzero, 0x1f80
    VUCOMISD_VALUE_CASE value_min_subnormal, value_pzero, 0x1fc0
    VUCOMISD_VALUE_CASE value_nmin_subnormal, value_pzero, 0x1fc0
    VUCOMISD_VALUE_CASE value_min_normal, value_min_subnormal, 0x1f80
    VUCOMISD_VALUE_CASE value_pone_high_snan, value_two_high_snan, 0x1f80
    VUCOMISD_VALUE_CASE value_pone, value_pone, 0x1fbf
    VUCOMISD_VALUE_CASE value_two, value_pone, 0x3f80
    VUCOMISD_VALUE_CASE value_two, value_pone, 0x5f80
    VUCOMISD_VALUE_CASE value_two, value_pone, 0x7f80
    VUCOMISD_VALUE_CASE value_min_subnormal, value_pzero, 0x9f80
    .endm

    .macro VUCOMISD_ALL_REGS
    VUCOMISD_REG_CASE xmm0, xmm1, value_pone, value_two, 0x1f80
    VUCOMISD_REG_CASE xmm1, xmm2, value_two, value_pone, 0x1f80
    VUCOMISD_REG_CASE xmm2, xmm3, value_pzero, value_nzero, 0x1f80
    VUCOMISD_REG_CASE xmm3, xmm4, value_qnan, value_pone, 0x1f80
    VUCOMISD_REG_CASE xmm4, xmm5, value_snan, value_pone, 0x1f80
    VUCOMISD_REG_CASE xmm5, xmm6, value_ntwo, value_none, 0x1f80
    VUCOMISD_REG_CASE xmm6, xmm7, value_none, value_ntwo, 0x1f80
    VUCOMISD_REG_CASE xmm7, xmm8, value_pinf, value_max_normal, 0x1f80
    VUCOMISD_REG_CASE xmm8, xmm9, value_ninf, value_nmax_normal, 0x1f80
    VUCOMISD_REG_CASE xmm9, xmm10, value_min_subnormal, value_pzero, 0x1f80
    VUCOMISD_REG_CASE xmm10, xmm11, value_min_subnormal, value_pzero, 0x1fc0
    VUCOMISD_REG_CASE xmm11, xmm12, value_pone, value_qnan_alt, 0x1f80
    VUCOMISD_REG_CASE xmm12, xmm13, value_pone, value_snan_alt, 0x1f80
    VUCOMISD_REG_CASE xmm13, xmm14, value_max_normal, value_pinf, 0x1f80
    VUCOMISD_REG_CASE xmm14, xmm15, value_nmax_normal, value_ninf, 0x1f80
    VUCOMISD_REG_CASE xmm15, xmm0, value_pone, value_pone, 0x1f80
    .endm

    .macro VUCOMISD_MEMORY_CASE left_reg, address_variant
    VUCOMISD_SET_MXCSR 0x1f80
    movdqu \left_reg, xmmword ptr [rip + vucomisd_memory_left]
    .if \address_variant == 0
    lea rbx, [rsi]
    .elseif \address_variant == 1
    lea rbx, [rsi + 8192]
    mov r12, 3
    lea rbx, [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    lea rbx, [rsi + 16384 + 120]
    .elseif \address_variant == 3
    lea rbx, [rsi + 25600]
    .elseif \address_variant == 4
    lea rbx, [rip + latx_avx_vucomisd_rip_source]
    .elseif \address_variant == 5
    lea rbx, [rsi + 32768 + 1]
    .elseif \address_variant == 6
    lea rbx, [rsi + 40960 + 4088]
    .else
    lea rbx, [rsi + 49152 + 4092]
    .endif
    VUCOMISD_SET_FLAGS
    vucomisd \left_reg, qword ptr [rbx]
    VUCOMISD_SAVE_RESULT \left_reg
    .endm

    .macro VUCOMISD_RUN_MEMORY left_reg
    .set vucomisd_memory_index, 0
    .rept 8
        VUCOMISD_MEMORY_CASE \left_reg, vucomisd_memory_index
        .set vucomisd_memory_index, vucomisd_memory_index + 1
    .endr
    .endm

    .globl latx_avx_single_vucomisd_run
    .type latx_avx_single_vucomisd_run, @function
latx_avx_single_vucomisd_run:
    push rbx
    push r12
    VUCOMISD_RUN_VALUES
    VUCOMISD_ALL_REGS
    VUCOMISD_RUN_MEMORY xmm0
    VUCOMISD_RUN_MEMORY xmm15

    VUCOMISD_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + value_two]
    VUCOMISD_SET_FLAGS
    vucomisd xmm0, xmm0
    VUCOMISD_SAVE_RESULT xmm0

    VUCOMISD_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + value_two]
    VUCOMISD_SET_FLAGS
    vucomisd xmm15, xmm15
    VUCOMISD_SAVE_RESULT xmm15

    VUCOMISD_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + value_two]
    movdqu xmm1, xmmword ptr [rip + value_pone]
    VUCOMISD_SET_FLAGS
    .byte 0xc5, 0xf9, 0x2e, 0xc1
    VUCOMISD_SAVE_RESULT xmm0

    VUCOMISD_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + value_pone]
    movdqu xmm1, xmmword ptr [rip + value_two]
    VUCOMISD_SET_FLAGS
    .byte 0xc4, 0xe1, 0xf9, 0x2e, 0xc1
    VUCOMISD_SAVE_RESULT xmm0

    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vucomisd_fault
    .type latx_avx_single_vucomisd_fault, @function
latx_avx_single_vucomisd_fault:
    VUCOMISD_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + vucomisd_fault_left]
    VUCOMISD_SET_FLAGS
    vucomisd xmm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vucomisd_fault_unmasked
    .type latx_avx_single_vucomisd_fault_unmasked, @function
latx_avx_single_vucomisd_fault_unmasked:
    VUCOMISD_SET_MXCSR 0x1f00
    movdqu xmm15, xmmword ptr [rip + value_snan]
    VUCOMISD_SET_FLAGS
    vucomisd xmm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vucomisd_unmasked
    .type latx_avx_single_vucomisd_unmasked, @function
latx_avx_single_vucomisd_unmasked:
    cmp esi, 1
    jne .Lvucomisd_unmasked_denormal
    VUCOMISD_SET_MXCSR 0x1f00
    movdqu xmm15, xmmword ptr [rip + value_snan]
    movdqu xmm14, xmmword ptr [rip + value_pone]
    jmp .Lvucomisd_unmasked_ready
.Lvucomisd_unmasked_denormal:
    cmp esi, 2
    jne .Lvucomisd_unmasked_invalid_priority
    VUCOMISD_SET_MXCSR 0x1e80
    movdqu xmm15, xmmword ptr [rip + value_min_subnormal]
    movdqu xmm14, xmmword ptr [rip + value_pzero]
    jmp .Lvucomisd_unmasked_ready
.Lvucomisd_unmasked_invalid_priority:
    cmp esi, 3
    jne .Lvucomisd_unmasked_denormal_priority
    VUCOMISD_SET_MXCSR 0x1e00
    movdqu xmm15, xmmword ptr [rip + value_snan]
    movdqu xmm14, xmmword ptr [rip + value_min_subnormal]
    jmp .Lvucomisd_unmasked_ready
.Lvucomisd_unmasked_denormal_priority:
    cmp esi, 4
    jne .Lvucomisd_no_signal_daz
    VUCOMISD_SET_MXCSR 0x1e80
    movdqu xmm15, xmmword ptr [rip + value_qnan]
    movdqu xmm14, xmmword ptr [rip + value_min_subnormal]
    jmp .Lvucomisd_unmasked_ready
.Lvucomisd_no_signal_daz:
    cmp esi, 5
    jne .Lvucomisd_no_signal_old_sticky
    VUCOMISD_SET_MXCSR 0x1ec0
    movdqu xmm15, xmmword ptr [rip + value_min_subnormal]
    movdqu xmm14, xmmword ptr [rip + value_pzero]
    jmp .Lvucomisd_unmasked_ready
.Lvucomisd_no_signal_old_sticky:
    cmp esi, 6
    jne .Lvucomisd_no_signal_invalid_suppresses_denormal
    VUCOMISD_SET_MXCSR 0x1e03
    movdqu xmm15, xmmword ptr [rip + value_two]
    movdqu xmm14, xmmword ptr [rip + value_pone]
    jmp .Lvucomisd_unmasked_ready
.Lvucomisd_no_signal_invalid_suppresses_denormal:
    VUCOMISD_SET_MXCSR 0x1e80
    movdqu xmm15, xmmword ptr [rip + value_snan]
    movdqu xmm14, xmmword ptr [rip + value_min_subnormal]
.Lvucomisd_unmasked_ready:
    VUCOMISD_SET_FLAGS

    .globl latx_avx_single_vucomisd_unmasked_site
latx_avx_single_vucomisd_unmasked_site:
    vucomisd xmm15, xmm14
    VUCOMISD_SAVE_RESULT xmm15
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 16
vucomisd_memory_left:
    .quad 0x4000000000000000, 0x13579bdf2468ace0
vucomisd_fault_left:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
value_pzero:
    .quad 0x0000000000000000, 0x13579bdf2468ace0
value_nzero:
    .quad 0x8000000000000000, 0x13579bdf2468ace0
value_pone:
    .quad 0x3ff0000000000000, 0x13579bdf2468ace0
value_none:
    .quad 0xbff0000000000000, 0x13579bdf2468ace0
value_two:
    .quad 0x4000000000000000, 0x13579bdf2468ace0
value_ntwo:
    .quad 0xc000000000000000, 0x13579bdf2468ace0
value_min_normal:
    .quad 0x0010000000000000, 0x13579bdf2468ace0
value_min_subnormal:
    .quad 0x0000000000000001, 0x13579bdf2468ace0
value_nmin_subnormal:
    .quad 0x8000000000000001, 0x13579bdf2468ace0
value_max_normal:
    .quad 0x7fefffffffffffff, 0x13579bdf2468ace0
value_nmax_normal:
    .quad 0xffefffffffffffff, 0x13579bdf2468ace0
value_pinf:
    .quad 0x7ff0000000000000, 0x13579bdf2468ace0
value_ninf:
    .quad 0xfff0000000000000, 0x13579bdf2468ace0
value_qnan:
    .quad 0x7ff8000000001234, 0x13579bdf2468ace0
value_snan:
    .quad 0x7ff0000000001234, 0x13579bdf2468ace0
value_qnan_alt:
    .quad 0xfff8000000005678, 0x13579bdf2468ace0
value_snan_alt:
    .quad 0xfff0000000005678, 0x13579bdf2468ace0
value_pone_high_snan:
    .quad 0x3ff0000000000000, 0x7ff0000000009876
value_two_high_snan:
    .quad 0x4000000000000000, 0xfff0000000008765

    .section .data
    .balign 16
    .globl latx_avx_vucomisd_rip_source
latx_avx_vucomisd_rip_source:
    .quad 0x7ff8000000005678

    .balign 4
vucomisd_mxcsr_control:
    .long 0x1f80

    .section .note.GNU-stack,"",@progbits
