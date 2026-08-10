/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VMULSD_RESTORE_YMM_SEED
    test r15d, r15d
    jz .Lvmulsd_skip_restore_\@
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vmulsd_xrstor_area]
.Lvmulsd_skip_restore_\@:
    .endm

    .macro VMULSD_SET_MXCSR control
    mov dword ptr [rip + vmulsd_mxcsr_control], \control
    ldmxcsr dword ptr [rip + vmulsd_mxcsr_control]
    .endm

    .macro VMULSD_SAVE_RESULT target
    test rdi, rdi
    jz .Lvmulsd_skip_save_\@
    .if \target == 0
    movdqu xmm12, xmm0
    .else
    movdqu xmm12, xmm15
    .endif
    movdqu xmmword ptr [rdi], xmm12
    test r15d, r15d
    jz .Lvmulsd_no_xsave_\@
    pxor xmm12, xmm12
    movdqu xmmword ptr [rip + vmulsd_xsave_area + 576 + \target * 16], \
        xmm12
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vmulsd_xsave_area]
    movdqu xmm12, \
        xmmword ptr [rip + vmulsd_xsave_area + 576 + \target * 16]
    jmp .Lvmulsd_have_high_\@
.Lvmulsd_no_xsave_\@:
    pxor xmm12, xmm12
.Lvmulsd_have_high_\@:
    movdqu xmmword ptr [rdi + 16], xmm12
    mov eax, dword ptr [rip + vmulsd_mxcsr_after]
    mov dword ptr [rdi + 32], eax
    mov dword ptr [rdi + 36], 0
    mov qword ptr [rdi + 40], 0
    mov qword ptr [rdi + 48], 0
    mov qword ptr [rdi + 56], 0
    add rdi, 64
.Lvmulsd_skip_save_\@:
    .endm

    .macro VMULSD_RECORD_RESULT target
    stmxcsr dword ptr [rip + vmulsd_mxcsr_after]
    movdqu xmm14, xmmword ptr [rip + value_pone]
    call latx_avx_single_vmulsd_observe_marker
    VMULSD_SAVE_RESULT \target
    .endm

    .macro VMULSD_ALIAS_CASES target
    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vmulsd_alias_all]
    vmulsd xmm0, xmm0, xmm0
    VMULSD_RECORD_RESULT 0

    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + vmulsd_alias_src1]
    movdqu xmm1, xmmword ptr [rip + vmulsd_alias_src2]
    vmulsd xmm0, xmm0, xmm1
    VMULSD_RECORD_RESULT 0

    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + vmulsd_alias_src2]
    movdqu xmm1, xmmword ptr [rip + vmulsd_alias_src1]
    vmulsd xmm0, xmm1, xmm0
    VMULSD_RECORD_RESULT 0

    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + vmulsd_target_seed]
    movdqu xmm1, xmmword ptr [rip + vmulsd_alias_src1]
    vmulsd xmm0, xmm1, xmm1
    VMULSD_RECORD_RESULT 0

    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + vmulsd_target_seed]
    movdqu xmm1, xmmword ptr [rip + vmulsd_alias_src1]
    movdqu xmm2, xmmword ptr [rip + vmulsd_alias_src2]
    vmulsd xmm0, xmm1, xmm2
    VMULSD_RECORD_RESULT 0
    .else
    movdqu xmm15, xmmword ptr [rip + vmulsd_alias_all]
    vmulsd xmm15, xmm15, xmm15
    VMULSD_RECORD_RESULT 15

    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + vmulsd_alias_src1]
    movdqu xmm14, xmmword ptr [rip + vmulsd_alias_src2]
    vmulsd xmm15, xmm15, xmm14
    VMULSD_RECORD_RESULT 15

    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + vmulsd_alias_src2]
    movdqu xmm14, xmmword ptr [rip + vmulsd_alias_src1]
    vmulsd xmm15, xmm14, xmm15
    VMULSD_RECORD_RESULT 15

    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + vmulsd_target_seed]
    movdqu xmm14, xmmword ptr [rip + vmulsd_alias_src1]
    vmulsd xmm15, xmm14, xmm14
    VMULSD_RECORD_RESULT 15

    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + vmulsd_target_seed]
    movdqu xmm13, xmmword ptr [rip + vmulsd_alias_src1]
    movdqu xmm14, xmmword ptr [rip + vmulsd_alias_src2]
    vmulsd xmm15, xmm13, xmm14
    VMULSD_RECORD_RESULT 15
    .endif
    .endm

    .macro VMULSD_VALUE_CASE target, src1, src2, control
    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR \control
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vmulsd_target_seed]
    movdqu xmm1, xmmword ptr [rip + \src1]
    movdqu xmm2, xmmword ptr [rip + \src2]
    vmulsd xmm0, xmm1, xmm2
    VMULSD_RECORD_RESULT 0
    .else
    movdqu xmm15, xmmword ptr [rip + vmulsd_target_seed]
    movdqu xmm13, xmmword ptr [rip + \src1]
    movdqu xmm14, xmmword ptr [rip + \src2]
    vmulsd xmm15, xmm13, xmm14
    VMULSD_RECORD_RESULT 15
    .endif
    .endm

    .macro VMULSD_RUN_VALUES target
    VMULSD_VALUE_CASE \target, value_pzero, value_pone, 0x1f80
    VMULSD_VALUE_CASE \target, value_nzero, value_pone, 0x1f80
    VMULSD_VALUE_CASE \target, value_pone, value_none, 0x1f80
    VMULSD_VALUE_CASE \target, value_onehalf, value_ntwohalf, 0x1f80
    VMULSD_VALUE_CASE \target, value_min_normal, value_half, 0x1f80
    VMULSD_VALUE_CASE \target, value_min_subnormal, value_pone, 0x1f80
    VMULSD_VALUE_CASE \target, value_pinf, value_pone, 0x1f80
    VMULSD_VALUE_CASE \target, value_ninf, value_pone, 0x1f80
    VMULSD_VALUE_CASE \target, value_pinf, value_pzero, 0x1f80
    VMULSD_VALUE_CASE \target, value_qnan, value_pone, 0x1f80
    VMULSD_VALUE_CASE \target, value_snan, value_pone, 0x1f80
    VMULSD_VALUE_CASE \target, value_min_subnormal, value_half, 0x1f80
    VMULSD_VALUE_CASE \target, value_max_normal, value_two, 0x1f80
    VMULSD_VALUE_CASE \target, value_max_normal, value_two, 0x3f80
    VMULSD_VALUE_CASE \target, value_max_normal, value_two, 0x5f80
    VMULSD_VALUE_CASE \target, value_max_normal, value_two, 0x7f80
    VMULSD_VALUE_CASE \target, value_nmax_normal, value_two, 0x3f80
    VMULSD_VALUE_CASE \target, value_nmax_normal, value_two, 0x5f80
    VMULSD_VALUE_CASE \target, value_pone, value_pone, 0x1fbf
    VMULSD_VALUE_CASE \target, value_min_subnormal, value_pone, 0x1fc0
    VMULSD_VALUE_CASE \target, value_min_normal, value_half, 0x9f80
    VMULSD_VALUE_CASE \target, value_min_subnormal, value_half, 0x9fc0
    VMULSD_VALUE_CASE \target, value_one_plus_ulp, value_one_plus_ulp, 0x1f80
    VMULSD_VALUE_CASE \target, value_one_plus_ulp, value_one_plus_ulp, 0x3f80
    VMULSD_VALUE_CASE \target, value_one_plus_ulp, value_one_plus_ulp, 0x5f80
    VMULSD_VALUE_CASE \target, value_one_plus_ulp, value_one_plus_ulp, 0x7f80
    VMULSD_VALUE_CASE \target, value_one_plus_ulp, value_none_plus_ulp, 0x1f80
    VMULSD_VALUE_CASE \target, value_one_plus_ulp, value_none_plus_ulp, 0x3f80
    VMULSD_VALUE_CASE \target, value_one_plus_ulp, value_none_plus_ulp, 0x5f80
    VMULSD_VALUE_CASE \target, value_one_plus_ulp, value_none_plus_ulp, 0x7f80
    VMULSD_VALUE_CASE \target, value_pone_high_snan, value_two_high_snan, 0x1f80
    VMULSD_VALUE_CASE \target, value_nmin_subnormal, value_pone, 0x1f80
    VMULSD_VALUE_CASE \target, value_nmin_normal, value_half, 0x1f80
    VMULSD_VALUE_CASE \target, value_nmin_subnormal, value_pone, 0x1fc0
    VMULSD_VALUE_CASE \target, value_nmin_normal, value_half, 0x9f80
    VMULSD_VALUE_CASE \target, value_qnan, value_qnan_alt, 0x1f80
    VMULSD_VALUE_CASE \target, value_qnan_alt, value_qnan, 0x1f80
    VMULSD_VALUE_CASE \target, value_snan, value_qnan_alt, 0x1f80
    VMULSD_VALUE_CASE \target, value_qnan_alt, value_snan, 0x1f80
    .endm

    .macro VMULSD_MEMORY_CASE target, address_variant
    VMULSD_RESTORE_YMM_SEED
    VMULSD_SET_MXCSR 0x1f80
    .if \target == 0
        .if (\address_variant & 1) == 0
        movdqu xmm0, xmmword ptr [rip + vmulsd_memory_src1]
        .else
        movdqu xmm0, xmmword ptr [rip + vmulsd_target_seed]
        movdqu xmm1, xmmword ptr [rip + vmulsd_memory_src1]
        .endif
    .else
        .if (\address_variant & 1) == 0
        movdqu xmm15, xmmword ptr [rip + vmulsd_memory_src1]
        .else
        movdqu xmm15, xmmword ptr [rip + vmulsd_target_seed]
        movdqu xmm14, xmmword ptr [rip + vmulsd_memory_src1]
        .endif
    .endif

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
    lea rbx, [rip + latx_avx_vmulsd_rip_source]
    .elseif \address_variant == 5
    lea rbx, [rsi + 32768 + 1]
    .elseif \address_variant == 6
    lea rbx, [rsi + 40960 + 4088]
    .else
    lea rbx, [rsi + 49152 + 4092]
    .endif

    .if \target == 0
        .if (\address_variant & 1) == 0
        vmulsd xmm0, xmm0, qword ptr [rbx]
        .else
        vmulsd xmm0, xmm1, qword ptr [rbx]
        .endif
        VMULSD_RECORD_RESULT 0
    .else
        .if (\address_variant & 1) == 0
        vmulsd xmm15, xmm15, qword ptr [rbx]
        .else
        vmulsd xmm15, xmm14, qword ptr [rbx]
        .endif
        VMULSD_RECORD_RESULT 15
    .endif
    .endm

    .macro VMULSD_RUN_REGISTERS target
    VMULSD_ALIAS_CASES \target
    VMULSD_RUN_VALUES \target
    .endm

    .macro VMULSD_RUN_MEMORY target
    .set vmulsd_memory_index, 0
    .rept 8
        VMULSD_MEMORY_CASE \target, vmulsd_memory_index
        .set vmulsd_memory_index, vmulsd_memory_index + 1
    .endr
    .endm

    .globl latx_avx_single_vmulsd_run
    .type latx_avx_single_vmulsd_run, @function
latx_avx_single_vmulsd_run:
    push rbx
    push r12
    push r15
    mov r15d, ecx
    test edx, edx
    jnz .Lvmulsd_selected_mode
    VMULSD_RUN_REGISTERS 0
    VMULSD_RUN_MEMORY 0
    VMULSD_RUN_REGISTERS 15
    VMULSD_RUN_MEMORY 15
    jmp .Lvmulsd_run_done
.Lvmulsd_selected_mode:
    cmp edx, 1
    jne .Lvmulsd_mode2
    VMULSD_RUN_REGISTERS 0
    jmp .Lvmulsd_run_done
.Lvmulsd_mode2:
    cmp edx, 2
    jne .Lvmulsd_mode3
    VMULSD_RUN_MEMORY 0
    jmp .Lvmulsd_run_done
.Lvmulsd_mode3:
    cmp edx, 3
    jne .Lvmulsd_mode4
    VMULSD_RUN_REGISTERS 15
    jmp .Lvmulsd_run_done
.Lvmulsd_mode4:
    VMULSD_RUN_MEMORY 15
.Lvmulsd_run_done:
    pop r15
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vmulsd_observe_marker
    .type latx_avx_single_vmulsd_observe_marker, @function
latx_avx_single_vmulsd_observe_marker:
    vmulsd xmm14, xmm14, xmm14
    vmulsd xmm14, xmm14, xmm14
    ret

    .globl latx_avx_single_vmulsd_fault
    .type latx_avx_single_vmulsd_fault, @function
latx_avx_single_vmulsd_fault:
    test esi, esi
    jz .Lvmulsd_fault_skip_restore
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vmulsd_xrstor_area]
.Lvmulsd_fault_skip_restore:
    VMULSD_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + vmulsd_fault_initial]
    movdqu xmm14, xmmword ptr [rip + vmulsd_fault_src1]
    vmulsd xmm15, xmm14, qword ptr [rdi]
    ret

    .globl latx_avx_single_vmulsd_fault_unmasked
    .type latx_avx_single_vmulsd_fault_unmasked, @function
latx_avx_single_vmulsd_fault_unmasked:
    test esi, esi
    jz .Lvmulsd_fault_unmasked_skip_restore
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vmulsd_xrstor_area]
.Lvmulsd_fault_unmasked_skip_restore:
    VMULSD_SET_MXCSR 0x1f00
    movdqu xmm15, xmmword ptr [rip + vmulsd_fault_initial]
    movdqu xmm14, xmmword ptr [rip + value_snan]
    vmulsd xmm15, xmm14, qword ptr [rdi]
    ret

    .globl latx_avx_single_vmulsd_unmasked
    .type latx_avx_single_vmulsd_unmasked, @function
latx_avx_single_vmulsd_unmasked:
    push r15
    mov r15d, edx
    VMULSD_RESTORE_YMM_SEED
    movdqu xmm15, xmmword ptr [rip + vmulsd_target_seed]

    cmp esi, 1
    jne .Lvmulsd_unmasked_denormal
    VMULSD_SET_MXCSR 0x1f00
    movdqu xmm13, xmmword ptr [rip + value_snan]
    movdqu xmm14, xmmword ptr [rip + value_pone]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_denormal:
    cmp esi, 2
    jne .Lvmulsd_unmasked_overflow
    VMULSD_SET_MXCSR 0x1e80
    movdqu xmm13, xmmword ptr [rip + value_min_subnormal]
    movdqu xmm14, xmmword ptr [rip + value_pone]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_overflow:
    cmp esi, 3
    jne .Lvmulsd_unmasked_underflow
    VMULSD_SET_MXCSR 0x1b80
    movdqu xmm13, xmmword ptr [rip + value_max_normal]
    movdqu xmm14, xmmword ptr [rip + value_two]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_underflow:
    cmp esi, 4
    jne .Lvmulsd_unmasked_precision
    VMULSD_SET_MXCSR 0x1780
    movdqu xmm13, xmmword ptr [rip + value_min_normal]
    movdqu xmm14, xmmword ptr [rip + value_half]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_precision:
    cmp esi, 5
    jne .Lvmulsd_unmasked_overflow_precision
    VMULSD_SET_MXCSR 0x0f80
    movdqu xmm13, xmmword ptr [rip + value_one_plus_ulp]
    movdqu xmm14, xmmword ptr [rip + value_one_plus_ulp]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_overflow_precision:
    cmp esi, 6
    jne .Lvmulsd_unmasked_underflow_precision
    VMULSD_SET_MXCSR 0x0f80
    movdqu xmm13, xmmword ptr [rip + value_max_normal]
    movdqu xmm14, xmmword ptr [rip + value_two]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_underflow_precision:
    cmp esi, 7
    jne .Lvmulsd_unmasked_denormal_priority
    VMULSD_SET_MXCSR 0x0f80
    movdqu xmm13, xmmword ptr [rip + value_min_subnormal]
    movdqu xmm14, xmmword ptr [rip + value_half]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_denormal_priority:
    cmp esi, 8
    jne .Lvmulsd_unmasked_invalid_priority
    VMULSD_SET_MXCSR 0x1680
    movdqu xmm13, xmmword ptr [rip + value_min_subnormal]
    movdqu xmm14, xmmword ptr [rip + value_half]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_invalid_priority:
    cmp esi, 9
    jne .Lvmulsd_unmasked_underflow_ftz
    VMULSD_SET_MXCSR 0x0f00
    movdqu xmm13, xmmword ptr [rip + value_snan]
    movdqu xmm14, xmmword ptr [rip + value_pone]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_underflow_ftz:
    cmp esi, 10
    jne .Lvmulsd_unmasked_daz
    VMULSD_SET_MXCSR 0x9780
    movdqu xmm13, xmmword ptr [rip + value_min_normal]
    movdqu xmm14, xmmword ptr [rip + value_half]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_daz:
    cmp esi, 11
    jne .Lvmulsd_unmasked_old_sticky
    VMULSD_SET_MXCSR 0x1ec0
    movdqu xmm13, xmmword ptr [rip + value_min_subnormal]
    movdqu xmm14, xmmword ptr [rip + value_pone]
    jmp latx_avx_single_vmulsd_unmasked_site
.Lvmulsd_unmasked_old_sticky:
    VMULSD_SET_MXCSR 0x1f01
    movdqu xmm13, xmmword ptr [rip + value_pone]
    movdqu xmm14, xmmword ptr [rip + value_pone]

    .globl latx_avx_single_vmulsd_unmasked_site
latx_avx_single_vmulsd_unmasked_site:
    vmulsd xmm15, xmm13, xmm14
    VMULSD_RECORD_RESULT 15
    pop r15
    ret

    .globl latx_avx_single_vmulsd_fault_observe
    .type latx_avx_single_vmulsd_fault_observe, @function
latx_avx_single_vmulsd_fault_observe:
    movdqu xmm14, xmmword ptr [rip + value_pone]
    jmp latx_avx_single_vmulsd_observe_marker

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 16
vmulsd_target_seed:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vmulsd_alias_all:
    .quad 0x4000000000000000, 0x1111222233334444
vmulsd_alias_src1:
    .quad 0x3ff8000000000000, 0x0123456789abcdef
vmulsd_alias_src2:
    .quad 0xc004000000000000, 0xfedcba9876543210
vmulsd_memory_src1:
    .quad 0x4000000000000000, 0x13579bdf2468ace0
vmulsd_fault_initial:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vmulsd_fault_src1:
    .quad 0x4000000000000000, 0x0f1e2d3c4b5a6978

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
value_two:
    .quad 0x4000000000000000, 0x13579bdf2468ace0
value_onehalf:
    .quad 0x3ff8000000000000, 0x13579bdf2468ace0
value_ntwohalf:
    .quad 0xc004000000000000, 0x13579bdf2468ace0
value_min_normal:
    .quad 0x0010000000000000, 0x13579bdf2468ace0
value_min_subnormal:
    .quad 0x0000000000000001, 0x13579bdf2468ace0
value_nmin_normal:
    .quad 0x8010000000000000, 0x13579bdf2468ace0
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
value_one_plus_ulp:
    .quad 0x3ff0000000000001, 0x13579bdf2468ace0
value_none_plus_ulp:
    .quad 0xbff0000000000001, 0x13579bdf2468ace0
value_pone_high_snan:
    .quad 0x3ff0000000000000, 0x7ff0000000009876
value_two_high_snan:
    .quad 0x4000000000000000, 0xfff0000000008765

    .section .data
    .balign 16
    .globl latx_avx_vmulsd_rip_source
latx_avx_vmulsd_rip_source:
    .quad 0x7ff8000000005678

    .balign 4
vmulsd_mxcsr_control:
    .long 0x1f80
vmulsd_mxcsr_after:
    .long 0

    .balign 64
vmulsd_xrstor_area:
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
vmulsd_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
