/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VDIVSD_RESTORE_YMM
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vdivsd_xrstor_area]
    .endm

    .macro VDIVSD_SET_MXCSR control
    mov dword ptr [rip + vdivsd_mxcsr_control], \control
    ldmxcsr dword ptr [rip + vdivsd_mxcsr_control]
    .endm

    .macro VDIVSD_SAVE target
    stmxcsr dword ptr [rip + vdivsd_mxcsr_after]
    pushfq
    pop r11
    call latx_avx_single_vdivsd_observe_marker
    test rdi, rdi
    jz .Lvdivsd_skip_save_\@
    .if \target == 0
    movdqu xmm12, xmm0
    .else
    movdqu xmm12, xmm15
    .endif
    movdqu xmmword ptr [rdi], xmm12
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vdivsd_xsave_area]
    movdqu xmm12, xmmword ptr [rip + vdivsd_xsave_area + 576 + \target * 16]
    movdqu xmmword ptr [rdi + 16], xmm12
    mov eax, dword ptr [rip + vdivsd_mxcsr_after]
    mov dword ptr [rdi + 32], eax
    mov dword ptr [rdi + 36], 0
    mov qword ptr [rdi + 40], r11
    mov qword ptr [rdi + 48], 0
    mov qword ptr [rdi + 56], 0
    add rdi, 64
.Lvdivsd_skip_save_\@:
    .endm

    .macro VDIVSD_REG target, src1, src2, control
    VDIVSD_RESTORE_YMM
    VDIVSD_SET_MXCSR \control
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vdivsd_target_seed]
    movdqu xmm1, xmmword ptr [rip + \src1]
    movdqu xmm2, xmmword ptr [rip + \src2]
    vdivsd xmm0, xmm1, xmm2
    .else
    movdqu xmm15, xmmword ptr [rip + vdivsd_target_seed]
    movdqu xmm14, xmmword ptr [rip + \src1]
    movdqu xmm13, xmmword ptr [rip + \src2]
    vdivsd xmm15, xmm14, xmm13
    .endif
    VDIVSD_SAVE \target
    .endm

    .macro VDIVSD_MEM target, offset, control
    VDIVSD_RESTORE_YMM
    VDIVSD_SET_MXCSR \control
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vdivsd_target_seed]
    movdqu xmm1, xmmword ptr [rip + vdivsd_src_seed]
    lea rbx, [rsi + \offset]
    vdivsd xmm0, xmm1, qword ptr [rbx]
    .else
    movdqu xmm15, xmmword ptr [rip + vdivsd_target_seed]
    movdqu xmm14, xmmword ptr [rip + vdivsd_src_seed]
    lea rbx, [rsi + \offset]
    vdivsd xmm15, xmm14, qword ptr [rbx]
    .endif
    VDIVSD_SAVE \target
    .endm

    .globl latx_avx_single_vdivsd_run
    .type latx_avx_single_vdivsd_run, @function
latx_avx_single_vdivsd_run:
    push rbx
    push r12
    push r15
    mov qword ptr [rip + vdivsd_output_ptr], rdi
    mov qword ptr [rip + vdivsd_output_start], rdi

    VDIVSD_REG 0, vdivsd_one, vdivsd_three, 0x1f80
    VDIVSD_REG 0, vdivsd_three, vdivsd_one, 0x1f80
    VDIVSD_REG 0, vdivsd_zero, vdivsd_zero, 0x1f80
    VDIVSD_REG 15, vdivsd_one, vdivsd_zero, 0x1f80
    VDIVSD_REG 15, vdivsd_min_normal, vdivsd_two, 0x1f80
    VDIVSD_REG 15, vdivsd_max_normal, vdivsd_half, 0x1f80
    VDIVSD_REG 0, vdivsd_qnan, vdivsd_one, 0x1f80
    VDIVSD_REG 15, vdivsd_snan, vdivsd_one, 0x1f80
    VDIVSD_REG 0, vdivsd_one, vdivsd_three, 0x3f80
    VDIVSD_REG 0, vdivsd_one, vdivsd_three, 0x5f80
    VDIVSD_REG 0, vdivsd_one, vdivsd_three, 0x7f80
    VDIVSD_REG 15, vdivsd_min_subnormal, vdivsd_two, 0x1f80

    VDIVSD_MEM 0, 1, 0x1f80
    VDIVSD_MEM 0, 4090, 0x1f80
    VDIVSD_MEM 15, 8193, 0x1f80
    VDIVSD_MEM 15, 12287, 0x1f80
    VDIVSD_MEM 0, 16385, 0x1f80
    VDIVSD_MEM 15, 20479, 0x1f80

    mov rax, qword ptr [rip + vdivsd_output_ptr]
    sub rax, qword ptr [rip + vdivsd_output_start]
    pop r15
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vdivsd_unmasked
    .type latx_avx_single_vdivsd_unmasked, @function
latx_avx_single_vdivsd_unmasked:
    VDIVSD_RESTORE_YMM
    cmp esi, 1
    jne .Lvdivsd_unmasked_divzero
    VDIVSD_SET_MXCSR 0x1f00
    movdqu xmm14, xmmword ptr [rip + vdivsd_snan]
    movdqu xmm13, xmmword ptr [rip + vdivsd_one]
    jmp latx_avx_single_vdivsd_unmasked_site
.Lvdivsd_unmasked_divzero:
    cmp esi, 2
    jne .Lvdivsd_unmasked_precision
    VDIVSD_SET_MXCSR 0x1d80
    movdqu xmm14, xmmword ptr [rip + vdivsd_one]
    movdqu xmm13, xmmword ptr [rip + vdivsd_zero]
    jmp latx_avx_single_vdivsd_unmasked_site
.Lvdivsd_unmasked_precision:
    VDIVSD_SET_MXCSR 0x0f80
    movdqu xmm14, xmmword ptr [rip + vdivsd_one]
    movdqu xmm13, xmmword ptr [rip + vdivsd_three]

    .globl latx_avx_single_vdivsd_unmasked_site
latx_avx_single_vdivsd_unmasked_site:
    vdivsd xmm15, xmm14, xmm13
    ret

    .globl latx_avx_single_vdivsd_observe_marker
latx_avx_single_vdivsd_observe_marker:
    ret

    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 16
vdivsd_target_seed: .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vdivsd_src_seed: .quad 0x0123456789abcdef, 0xfedcba9876543210
vdivsd_one: .quad 0x3ff0000000000000, 0x0123456789abcdef
vdivsd_two: .quad 0x4000000000000000, 0xfedcba9876543210
vdivsd_three: .quad 0x4008000000000000, 0x1122334455667788
vdivsd_half: .quad 0x3fe0000000000000, 0x99aabbccddeeff00
vdivsd_zero: .quad 0, 0x8877665544332211
vdivsd_min_normal: .quad 0x0010000000000000, 0x0123456789abcdef
vdivsd_min_subnormal: .quad 1, 0xfedcba9876543210
vdivsd_max_normal: .quad 0x7fefffffffffffff, 0
vdivsd_qnan: .quad 0x7ff8000000001234, 0
vdivsd_snan: .quad 0x7ff0000000001234, 0

    .section .data
    .balign 64
vdivsd_xrstor_area:
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
vdivsd_xsave_area: .zero 4096
vdivsd_output_ptr: .zero 8
vdivsd_output_start: .zero 8
vdivsd_mxcsr_control: .zero 4
vdivsd_mxcsr_after: .zero 4
    .section .note.GNU-stack,"",@progbits
