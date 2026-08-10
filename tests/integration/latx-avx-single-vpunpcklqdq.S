/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VPUNPCKLQDQ_RESTORE_SEED
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vpunpcklqdq_xrstor_area]
    movdqu xmm15, xmmword ptr [rip + vpunpcklqdq_seed_low]
    vpunpcklqdq ymm13, ymm15, ymmword ptr [rip + vpunpcklqdq_all_zeros]
    .endm

    .macro VPUNPCKLQDQ_SETUP_REGS index
    vpunpcklqdq ymm0, ymm13, ymmword ptr [rip + vpunpcklqdq_masks + ((\index % 6) * 32)]
    vpunpcklqdq ymm1, ymm13, ymmword ptr [rip + vpunpcklqdq_masks + (((\index + 1) % 6) * 32)]
    vpunpcklqdq ymm2, ymm13, ymmword ptr [rip + vpunpcklqdq_masks + (((\index + 2) % 6) * 32)]
    vpunpcklqdq ymm15, ymm13, ymmword ptr [rip + vpunpcklqdq_masks + (((\index + 3) % 6) * 32)]
    .endm

    .macro VPUNPCKLQDQ_SAVE_RESULT
    test rdi, rdi
    jz .Lvpunpcklqdq_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vpunpcklqdq_xsave_area + 160 + 15 * 16], xmm14
    movdqu xmmword ptr [rip + vpunpcklqdq_xsave_area + 576 + 15 * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpunpcklqdq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpunpcklqdq_xsave_area + 160 + 15 * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + vpunpcklqdq_xsave_area + 576 + 15 * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvpunpcklqdq_skip_save_\@:
    .endm

    .macro VPUNPCKLQDQ_REG_CASE width, variant
    VPUNPCKLQDQ_SETUP_REGS \variant
    .if \width == 128
        .if \variant == 0
        /* ToDesk+0xdb69c4: vpunpcklqdq xmm0,xmm0,xmm1. */
        .byte 0xc5, 0xf9, 0x6c, 0xc1
        .elseif \variant == 1
        vpunpcklqdq xmm0, xmm0, xmm2
        .elseif \variant == 2
        vpunpcklqdq xmm0, xmm1, xmm0
        .elseif \variant == 3
        vpunpcklqdq xmm0, xmm1, xmm1
        .elseif \variant == 4
        vpunpcklqdq xmm0, xmm0, xmm0
        .elseif \variant == 5
        vpunpcklqdq xmm15, xmm1, xmm2
        .elseif \variant == 6
        vpunpcklqdq xmm15, xmm15, xmm2
        .elseif \variant == 7
        vpunpcklqdq xmm15, xmm1, xmm15
        .elseif \variant == 8
        vpunpcklqdq xmm15, xmm1, xmm1
        .else
        vpunpcklqdq xmm15, xmm15, xmm15
        .endif
    .else
        .if \variant == 0
        vpunpcklqdq ymm0, ymm1, ymm2
        .elseif \variant == 1
        vpunpcklqdq ymm0, ymm0, ymm2
        .elseif \variant == 2
        vpunpcklqdq ymm0, ymm1, ymm0
        .elseif \variant == 3
        vpunpcklqdq ymm0, ymm1, ymm1
        .elseif \variant == 4
        vpunpcklqdq ymm0, ymm0, ymm0
        .elseif \variant == 5
        vpunpcklqdq ymm15, ymm1, ymm2
        .elseif \variant == 6
        vpunpcklqdq ymm15, ymm15, ymm2
        .elseif \variant == 7
        vpunpcklqdq ymm15, ymm1, ymm15
        .elseif \variant == 8
        vpunpcklqdq ymm15, ymm1, ymm1
        .else
        vpunpcklqdq ymm15, ymm15, ymm15
        .endif
    .endif
    .if \variant < 5
    call latx_avx_single_vpunpcklqdq_observe_ymm0
    .else
    call latx_avx_single_vpunpcklqdq_observe_ymm15
    .endif
    VPUNPCKLQDQ_SAVE_RESULT
    .endm

    .macro VPUNPCKLQDQ_PREPARE_ADDRESS width, address_variant
    .if \address_variant == 0
    lea rbx, [rsi]
    .elseif \address_variant == 1
    lea rbx, [rsi + 8192]
    mov r12, 3
    .elseif \address_variant == 2
    lea rbx, [rsi + 16384]
    .elseif \address_variant == 4
    lea rbx, [rsi + 32768]
    .elseif \address_variant == 5
    lea rbx, [rsi + 40960]
    .elseif \address_variant == 6
    lea rbx, [rsi + 49152]
    .endif
    .endm

    .macro VPUNPCKLQDQ_XMM_MEM dest, src, address_variant
    .if \address_variant == 0
    vpunpcklqdq \dest, \src, xmmword ptr [rbx]
    .elseif \address_variant == 1
    vpunpcklqdq \dest, \src, xmmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpunpcklqdq \dest, \src, xmmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpunpcklqdq \dest, \src, xmmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpunpcklqdq \dest, \src, xmmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpunpcklqdq \dest, \src, xmmword ptr [rbx + 4080]
    .elseif \address_variant == 6
    vpunpcklqdq \dest, \src, xmmword ptr [rbx + 4090]
    .else
    vpunpcklqdq \dest, \src, xmmword ptr [rip + vpunpcklqdq_rip_source]
    .endif
    .endm

    .macro VPUNPCKLQDQ_YMM_MEM dest, src, address_variant
    .if \address_variant == 0
    vpunpcklqdq \dest, \src, ymmword ptr [rbx]
    .elseif \address_variant == 1
    vpunpcklqdq \dest, \src, ymmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpunpcklqdq \dest, \src, ymmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpunpcklqdq \dest, \src, ymmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpunpcklqdq \dest, \src, ymmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpunpcklqdq \dest, \src, ymmword ptr [rbx + 4064]
    .elseif \address_variant == 6
    vpunpcklqdq \dest, \src, ymmword ptr [rbx + 4080]
    .else
    vpunpcklqdq \dest, \src, ymmword ptr [rip + vpunpcklqdq_rip_source]
    .endif
    .endm

    .macro VPUNPCKLQDQ_MEM_CASE width, index
    VPUNPCKLQDQ_SETUP_REGS \index
    VPUNPCKLQDQ_PREPARE_ADDRESS \width, (\index % 8)
    .if \width == 128
        .if (\index % 4) == 0
        VPUNPCKLQDQ_XMM_MEM xmm0, xmm1, (\index % 8)
        .elseif (\index % 4) == 1
        VPUNPCKLQDQ_XMM_MEM xmm0, xmm0, (\index % 8)
        .elseif (\index % 4) == 2
        VPUNPCKLQDQ_XMM_MEM xmm15, xmm1, (\index % 8)
        .else
        VPUNPCKLQDQ_XMM_MEM xmm15, xmm15, (\index % 8)
        .endif
    .else
        .if (\index % 4) == 0
        VPUNPCKLQDQ_YMM_MEM ymm0, ymm1, (\index % 8)
        .elseif (\index % 4) == 1
        VPUNPCKLQDQ_YMM_MEM ymm0, ymm0, (\index % 8)
        .elseif (\index % 4) == 2
        VPUNPCKLQDQ_YMM_MEM ymm15, ymm1, (\index % 8)
        .else
        VPUNPCKLQDQ_YMM_MEM ymm15, ymm15, (\index % 8)
        .endif
    .endif
    .if (\index % 4) < 2
    call latx_avx_single_vpunpcklqdq_observe_ymm0
    .else
    call latx_avx_single_vpunpcklqdq_observe_ymm15
    .endif
    VPUNPCKLQDQ_SAVE_RESULT
    .endm

    .globl latx_avx_single_vpunpcklqdq_run
    .type latx_avx_single_vpunpcklqdq_run, @function
latx_avx_single_vpunpcklqdq_run:
    push rbx
    push r12
    VPUNPCKLQDQ_RESTORE_SEED

    .set vpunpcklqdq_reg_variant, 0
    .rept 10
        VPUNPCKLQDQ_REG_CASE 128, vpunpcklqdq_reg_variant
        .set vpunpcklqdq_reg_variant, vpunpcklqdq_reg_variant + 1
    .endr
    .set vpunpcklqdq_reg_variant, 0
    .rept 10
        VPUNPCKLQDQ_REG_CASE 256, vpunpcklqdq_reg_variant
        .set vpunpcklqdq_reg_variant, vpunpcklqdq_reg_variant + 1
    .endr

    .set vpunpcklqdq_mem_index, 0
    .rept 32
        VPUNPCKLQDQ_MEM_CASE 128, vpunpcklqdq_mem_index
        .set vpunpcklqdq_mem_index, vpunpcklqdq_mem_index + 1
    .endr
    .set vpunpcklqdq_mem_index, 0
    .rept 32
        VPUNPCKLQDQ_MEM_CASE 256, vpunpcklqdq_mem_index
        .set vpunpcklqdq_mem_index, vpunpcklqdq_mem_index + 1
    .endr

    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vpunpcklqdq_flags_preserved
    .type latx_avx_single_vpunpcklqdq_flags_preserved, @function
latx_avx_single_vpunpcklqdq_flags_preserved:
    sub rsp, 16
    pushfq
    pop rax
    stmxcsr dword ptr [rsp]
    movdqu xmm0, xmmword ptr [rip + vpunpcklqdq_seed_low]
    movdqu xmm1, xmmword ptr [rip + vpunpcklqdq_masks]
    /* ToDesk+0xdb69c4: vpunpcklqdq xmm0,xmm0,xmm1. */
    .byte 0xc5, 0xf9, 0x6c, 0xc1
    pushfq
    pop rdx
    stmxcsr dword ptr [rsp + 4]
    cmp rax, rdx
    jne .Lvpunpcklqdq_flags_changed
    mov ecx, dword ptr [rsp]
    cmp ecx, dword ptr [rsp + 4]
    jne .Lvpunpcklqdq_flags_changed
    mov eax, 1
    add rsp, 16
    ret
.Lvpunpcklqdq_flags_changed:
    xor eax, eax
    add rsp, 16
    ret

    .globl latx_avx_single_vpunpcklqdq_observe_ymm0
    .type latx_avx_single_vpunpcklqdq_observe_ymm0, @function
latx_avx_single_vpunpcklqdq_observe_ymm0:
    vpunpcklqdq ymm12, ymm0, ymmword ptr [rip + vpunpcklqdq_all_zeros]
    jmp .Lvpunpcklqdq_observe_common

    .globl latx_avx_single_vpunpcklqdq_observe_ymm15
    .type latx_avx_single_vpunpcklqdq_observe_ymm15, @function
latx_avx_single_vpunpcklqdq_observe_ymm15:
    vpunpcklqdq ymm12, ymm15, ymmword ptr [rip + vpunpcklqdq_all_zeros]
.Lvpunpcklqdq_observe_common:
    .globl latx_avx_single_vpunpcklqdq_observe_marker
latx_avx_single_vpunpcklqdq_observe_marker:
    vpunpcklqdq ymm15, ymm12, ymmword ptr [rip + vpunpcklqdq_all_zeros]
    vpunpcklqdq ymm14, ymm14, ymmword ptr [rip + vpunpcklqdq_all_zeros]
    ret

    .macro VPUNPCKLQDQ_FAULT_PREPARE
    VPUNPCKLQDQ_RESTORE_SEED
    vpunpcklqdq ymm15, ymm13, ymmword ptr [rip + vpunpcklqdq_all_zeros]
    .endm

    .globl latx_avx_single_vpunpcklqdq_fault_xmm
    .type latx_avx_single_vpunpcklqdq_fault_xmm, @function
latx_avx_single_vpunpcklqdq_fault_xmm:
    VPUNPCKLQDQ_FAULT_PREPARE
    vpunpcklqdq xmm15, xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vpunpcklqdq_fault_ymm
    .type latx_avx_single_vpunpcklqdq_fault_ymm, @function
latx_avx_single_vpunpcklqdq_fault_ymm:
    VPUNPCKLQDQ_FAULT_PREPARE
    vpunpcklqdq ymm15, ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vpunpcklqdq_fault_observe
    .type latx_avx_single_vpunpcklqdq_fault_observe, @function
latx_avx_single_vpunpcklqdq_fault_observe:
    jmp latx_avx_single_vpunpcklqdq_observe_ymm15

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vpunpcklqdq_seed_low:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vpunpcklqdq_all_zeros:
    .quad 0, 0, 0, 0
vpunpcklqdq_masks:
    .quad 0, 0, 0, 0
    .quad -1, -1, -1, -1
    .quad 0x8000000000000000, 0, 0x8000000000000000, 0
    .byte 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    .byte 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    .byte 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
    .byte 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    .quad 0x55aa55aa55aa55aa, 0xaa55aa55aa55aa55
    .quad 0x55aa55aa55aa55aa, 0xaa55aa55aa55aa55
    .quad 0xd739a2f14c806b5e, 0x07e6d5c4b3a29180
    .quad 0x89abcdef01234567, 0xfedcba9876543210
vpunpcklqdq_rip_source:
    .quad 0, -1, 0x8000000000000000, 0x55aa55aa55aa55aa

    .section .data
    .balign 64
vpunpcklqdq_xrstor_area:
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
    .zero 240
    .quad 0x1122334455667788, 0x99aabbccddeeff00

    .section .bss
    .balign 64
vpunpcklqdq_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
