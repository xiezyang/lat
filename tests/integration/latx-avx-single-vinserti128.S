/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VINSERT_SETUP imm
    lea rbx, [rip + vinserti128_vectors]
    movdqu xmm0, xmmword ptr [rbx + ((\imm) % 6) * 16]
    movdqu xmm14, xmmword ptr [rbx + (((\imm) + 1) % 6) * 16]
    vinserti128 ymm0, ymm0, xmm14, 1
    movdqu xmm15, xmmword ptr [rbx + (((\imm) + 2) % 6) * 16]
    movdqu xmm14, xmmword ptr [rbx + (((\imm) + 3) % 6) * 16]
    vinserti128 ymm15, ymm15, xmm14, 1
    movdqu xmm2, xmmword ptr [rbx + (((\imm) + 4) % 6) * 16]
    .endm

    .macro VINSERT_OBSERVE_RESULT
    vinserti128 ymm14, ymm14, xmm14, 255
    .endm

    .macro VINSERT_SAVE_RESULT reg
    test rdi, rdi
    jz .Lvinsert_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vinserti128_xsave_area + 160 + (\reg) * 16], xmm14
    movdqu xmmword ptr [rip + vinserti128_xsave_area + 576 + (\reg) * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vinserti128_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vinserti128_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + vinserti128_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvinsert_skip_save_\@:
    .endm

    .macro VINSERT_REG_CASE imm, variant
    VINSERT_SETUP \imm
    .if \variant == 0
    vinserti128 ymm15, ymm0, xmm2, \imm
    .elseif \variant == 1
    vinserti128 ymm15, ymm15, xmm2, \imm
    .elseif \variant == 2
    vinserti128 ymm15, ymm0, xmm15, \imm
    .elseif \variant == 3
    vinserti128 ymm15, ymm0, xmm0, \imm
    .elseif \variant == 4
    vinserti128 ymm15, ymm15, xmm15, \imm
    .elseif \variant == 5
    vinserti128 ymm0, ymm15, xmm2, \imm
    .elseif \variant == 6
    vinserti128 ymm0, ymm0, xmm2, \imm
    .elseif \variant == 7
    vinserti128 ymm0, ymm15, xmm0, \imm
    .elseif \variant == 8
    vinserti128 ymm0, ymm15, xmm15, \imm
    .else
    vinserti128 ymm0, ymm0, xmm0, \imm
    .endif
    VINSERT_OBSERVE_RESULT
    .if \variant < 5
    VINSERT_SAVE_RESULT 15
    .else
    VINSERT_SAVE_RESULT 0
    .endif
    .endm

    .macro VINSERT_MEM_CASE imm, address_variant, target_variant
    VINSERT_SETUP \imm
    .if \target_variant == 0
        .if \address_variant == 0
        vinserti128 ymm15, ymm0, xmmword ptr [rsi + 1], \imm
        .elseif \address_variant == 1
        vinserti128 ymm15, ymm0, xmmword ptr [rsi + r12 * 2 + 7], \imm
        .elseif \address_variant == 2
        vinserti128 ymm15, ymm0, xmmword ptr [rsi + 120], \imm
        .elseif \address_variant == 3
        vinserti128 ymm15, ymm0, xmmword ptr [rsi + 1024], \imm
        .elseif \address_variant == 4
        vinserti128 ymm15, ymm0, xmmword ptr [rsi + 4080], \imm
        .elseif \address_variant == 5
        vinserti128 ymm15, ymm0, xmmword ptr [rsi + 4090], \imm
        .elseif \address_variant == 6
        vinserti128 ymm15, ymm0, xmmword ptr [rsi + 8176], \imm
        .else
        vinserti128 ymm15, ymm0, xmmword ptr [rip + vinserti128_rip_source], \imm
        .endif
    .elseif \target_variant == 1
        .if \address_variant == 0
        vinserti128 ymm15, ymm15, xmmword ptr [rsi + 1], \imm
        .elseif \address_variant == 1
        vinserti128 ymm15, ymm15, xmmword ptr [rsi + r12 * 2 + 7], \imm
        .elseif \address_variant == 2
        vinserti128 ymm15, ymm15, xmmword ptr [rsi + 120], \imm
        .elseif \address_variant == 3
        vinserti128 ymm15, ymm15, xmmword ptr [rsi + 1024], \imm
        .elseif \address_variant == 4
        vinserti128 ymm15, ymm15, xmmword ptr [rsi + 4080], \imm
        .elseif \address_variant == 5
        vinserti128 ymm15, ymm15, xmmword ptr [rsi + 4090], \imm
        .elseif \address_variant == 6
        vinserti128 ymm15, ymm15, xmmword ptr [rsi + 8176], \imm
        .else
        vinserti128 ymm15, ymm15, xmmword ptr [rip + vinserti128_rip_source], \imm
        .endif
    .elseif \target_variant == 2
        .if \address_variant == 0
        vinserti128 ymm0, ymm15, xmmword ptr [rsi + 1], \imm
        .elseif \address_variant == 1
        vinserti128 ymm0, ymm15, xmmword ptr [rsi + r12 * 2 + 7], \imm
        .elseif \address_variant == 2
        vinserti128 ymm0, ymm15, xmmword ptr [rsi + 120], \imm
        .elseif \address_variant == 3
        vinserti128 ymm0, ymm15, xmmword ptr [rsi + 1024], \imm
        .elseif \address_variant == 4
        vinserti128 ymm0, ymm15, xmmword ptr [rsi + 4080], \imm
        .elseif \address_variant == 5
        vinserti128 ymm0, ymm15, xmmword ptr [rsi + 4090], \imm
        .elseif \address_variant == 6
        vinserti128 ymm0, ymm15, xmmword ptr [rsi + 8176], \imm
        .else
        vinserti128 ymm0, ymm15, xmmword ptr [rip + vinserti128_rip_source], \imm
        .endif
    .else
        .if \address_variant == 0
        vinserti128 ymm0, ymm0, xmmword ptr [rsi + 1], \imm
        .elseif \address_variant == 1
        vinserti128 ymm0, ymm0, xmmword ptr [rsi + r12 * 2 + 7], \imm
        .elseif \address_variant == 2
        vinserti128 ymm0, ymm0, xmmword ptr [rsi + 120], \imm
        .elseif \address_variant == 3
        vinserti128 ymm0, ymm0, xmmword ptr [rsi + 1024], \imm
        .elseif \address_variant == 4
        vinserti128 ymm0, ymm0, xmmword ptr [rsi + 4080], \imm
        .elseif \address_variant == 5
        vinserti128 ymm0, ymm0, xmmword ptr [rsi + 4090], \imm
        .elseif \address_variant == 6
        vinserti128 ymm0, ymm0, xmmword ptr [rsi + 8176], \imm
        .else
        vinserti128 ymm0, ymm0, xmmword ptr [rip + vinserti128_rip_source], \imm
        .endif
    .endif
    VINSERT_OBSERVE_RESULT
    .if \target_variant < 2
    VINSERT_SAVE_RESULT 15
    .else
    VINSERT_SAVE_RESULT 0
    .endif
    .endm

    .globl latx_avx_single_vinserti128_run
    .type latx_avx_single_vinserti128_run, @function
latx_avx_single_vinserti128_run:
    push rbx
    push r12
    mov r12, 3

    .set vinsert_reg_imm, 0
    .rept 256
        VINSERT_REG_CASE vinsert_reg_imm, (vinsert_reg_imm % 10)
        .set vinsert_reg_imm, vinsert_reg_imm + 1
    .endr

    .set vinsert_mem_imm, 0
    .rept 256
        VINSERT_MEM_CASE vinsert_mem_imm, (vinsert_mem_imm % 8), \
                         (vinsert_mem_imm % 4)
        .set vinsert_mem_imm, vinsert_mem_imm + 1
    .endr

    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vinserti128_fault_cross
    .type latx_avx_single_vinserti128_fault_cross, @function
latx_avx_single_vinserti128_fault_cross:
    movdqu xmm15, xmmword ptr [rip + vinserti128_fault_low]
    movdqu xmm14, xmmword ptr [rip + vinserti128_fault_high]
    vinserti128 ymm15, ymm15, xmm14, 1
    lea rax, [rdi + 4090]
    vinserti128 ymm15, ymm15, xmmword ptr [rax], 1
    ret

    .globl latx_avx_single_vinserti128_fault_observe
    .type latx_avx_single_vinserti128_fault_observe, @function
latx_avx_single_vinserti128_fault_observe:
    movdqu xmm14, xmmword ptr [rip + vinserti128_rip_source]
    vinserti128 ymm14, ymm14, xmm14, 0
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vinserti128_vectors:
    .quad 0, 0
    .quad -1, -1
    .quad 0x8000000000000000, 0x0000000000000000
    .byte 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    .byte 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    .quad 0x55aa55aa55aa55aa, 0xaa55aa55aa55aa55
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vinserti128_rip_source:
    .quad 0x8877665544332211, 0x0f1e2d3c4b5a6978
vinserti128_fault_low:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vinserti128_fault_high:
    .quad 0x1122334455667788, 0x99aabbccddeeff00

    .bss
    .balign 64
vinserti128_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
