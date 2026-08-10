/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VPOR_RESTORE_SEED
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vpor_xrstor_area]
    movdqu xmm15, xmmword ptr [rip + vpor_seed_low]
    vpor ymm13, ymm15, ymmword ptr [rip + vpor_all_zeros]
    .endm

    .macro VPOR_SETUP_REGS index
    vpor ymm0, ymm13, ymmword ptr [rip + vpor_masks + ((\index % 6) * 32)]
    vpor ymm1, ymm13, ymmword ptr [rip + vpor_masks + (((\index + 1) % 6) * 32)]
    vpor ymm2, ymm13, ymmword ptr [rip + vpor_masks + (((\index + 2) % 6) * 32)]
    vpor ymm15, ymm13, ymmword ptr [rip + vpor_masks + (((\index + 3) % 6) * 32)]
    .endm

    .macro VPOR_SAVE_RESULT
    test rdi, rdi
    jz .Lvpor_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vpor_xsave_area + 160 + 15 * 16], xmm14
    movdqu xmmword ptr [rip + vpor_xsave_area + 576 + 15 * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpor_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpor_xsave_area + 160 + 15 * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + vpor_xsave_area + 576 + 15 * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvpor_skip_save_\@:
    .endm

    .macro VPOR_REG_CASE width, variant
    VPOR_SETUP_REGS \variant
    .if \width == 128
        .if \variant == 0
        vpor xmm0, xmm1, xmm2
        .elseif \variant == 1
        vpor xmm0, xmm0, xmm2
        .elseif \variant == 2
        vpor xmm0, xmm1, xmm0
        .elseif \variant == 3
        vpor xmm0, xmm1, xmm1
        .elseif \variant == 4
        vpor xmm0, xmm0, xmm0
        .elseif \variant == 5
        vpor xmm15, xmm1, xmm2
        .elseif \variant == 6
        vpor xmm15, xmm15, xmm2
        .elseif \variant == 7
        vpor xmm15, xmm1, xmm15
        .elseif \variant == 8
        vpor xmm15, xmm1, xmm1
        .else
        vpor xmm15, xmm15, xmm15
        .endif
    .else
        .if \variant == 0
        vpor ymm0, ymm1, ymm2
        .elseif \variant == 1
        vpor ymm0, ymm0, ymm2
        .elseif \variant == 2
        vpor ymm0, ymm1, ymm0
        .elseif \variant == 3
        vpor ymm0, ymm1, ymm1
        .elseif \variant == 4
        vpor ymm0, ymm0, ymm0
        .elseif \variant == 5
        vpor ymm15, ymm1, ymm2
        .elseif \variant == 6
        vpor ymm15, ymm15, ymm2
        .elseif \variant == 7
        vpor ymm15, ymm1, ymm15
        .elseif \variant == 8
        vpor ymm15, ymm1, ymm1
        .else
        vpor ymm15, ymm15, ymm15
        .endif
    .endif
    .if \variant < 5
    call latx_avx_single_vpor_observe_ymm0
    .else
    call latx_avx_single_vpor_observe_ymm15
    .endif
    VPOR_SAVE_RESULT
    .endm

    .macro VPOR_PREPARE_ADDRESS width, address_variant
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

    .macro VPOR_XMM_MEM dest, src, address_variant
    .if \address_variant == 0
    vpor \dest, \src, xmmword ptr [rbx]
    .elseif \address_variant == 1
    vpor \dest, \src, xmmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpor \dest, \src, xmmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpor \dest, \src, xmmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpor \dest, \src, xmmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpor \dest, \src, xmmword ptr [rbx + 4080]
    .elseif \address_variant == 6
    vpor \dest, \src, xmmword ptr [rbx + 4090]
    .else
    vpor \dest, \src, xmmword ptr [rip + vpor_rip_source]
    .endif
    .endm

    .macro VPOR_YMM_MEM dest, src, address_variant
    .if \address_variant == 0
    vpor \dest, \src, ymmword ptr [rbx]
    .elseif \address_variant == 1
    vpor \dest, \src, ymmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpor \dest, \src, ymmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpor \dest, \src, ymmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpor \dest, \src, ymmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpor \dest, \src, ymmword ptr [rbx + 4064]
    .elseif \address_variant == 6
    vpor \dest, \src, ymmword ptr [rbx + 4080]
    .else
    vpor \dest, \src, ymmword ptr [rip + vpor_rip_source]
    .endif
    .endm

    .macro VPOR_MEM_CASE width, index
    VPOR_SETUP_REGS \index
    VPOR_PREPARE_ADDRESS \width, (\index % 8)
    .if \width == 128
        .if (\index % 4) == 0
        VPOR_XMM_MEM xmm0, xmm1, (\index % 8)
        .elseif (\index % 4) == 1
        VPOR_XMM_MEM xmm0, xmm0, (\index % 8)
        .elseif (\index % 4) == 2
        VPOR_XMM_MEM xmm15, xmm1, (\index % 8)
        .else
        VPOR_XMM_MEM xmm15, xmm15, (\index % 8)
        .endif
    .else
        .if (\index % 4) == 0
        VPOR_YMM_MEM ymm0, ymm1, (\index % 8)
        .elseif (\index % 4) == 1
        VPOR_YMM_MEM ymm0, ymm0, (\index % 8)
        .elseif (\index % 4) == 2
        VPOR_YMM_MEM ymm15, ymm1, (\index % 8)
        .else
        VPOR_YMM_MEM ymm15, ymm15, (\index % 8)
        .endif
    .endif
    .if (\index % 4) < 2
    call latx_avx_single_vpor_observe_ymm0
    .else
    call latx_avx_single_vpor_observe_ymm15
    .endif
    VPOR_SAVE_RESULT
    .endm

    .globl latx_avx_single_vpor_run
    .type latx_avx_single_vpor_run, @function
latx_avx_single_vpor_run:
    push rbx
    push r12
    VPOR_RESTORE_SEED

    .set vpor_reg_variant, 0
    .rept 10
        VPOR_REG_CASE 128, vpor_reg_variant
        .set vpor_reg_variant, vpor_reg_variant + 1
    .endr
    .set vpor_reg_variant, 0
    .rept 10
        VPOR_REG_CASE 256, vpor_reg_variant
        .set vpor_reg_variant, vpor_reg_variant + 1
    .endr

    .set vpor_mem_index, 0
    .rept 32
        VPOR_MEM_CASE 128, vpor_mem_index
        .set vpor_mem_index, vpor_mem_index + 1
    .endr
    .set vpor_mem_index, 0
    .rept 32
        VPOR_MEM_CASE 256, vpor_mem_index
        .set vpor_mem_index, vpor_mem_index + 1
    .endr

    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vpor_logic
    .type latx_avx_single_vpor_logic, @function
latx_avx_single_vpor_logic:
    movdqu xmm0, xmmword ptr [rip + vpor_seed_low]
    movdqu xmm1, xmmword ptr [rip + vpor_logic_mask]
    vpor xmm2, xmm0, xmm1
    movdqu xmmword ptr [rdi], xmm2
    movdqu xmm3, xmmword ptr [rip + vpor_seed_low]
    vpor xmm4, xmm3, xmm1
    movdqu xmmword ptr [rdi + 16], xmm4
    vpor ymm6, ymm6, ymm6
    vpor ymm6, ymm6, ymmword ptr [rip + vpor_logic_mask]
    ret

    .globl latx_avx_single_vpor_observe_ymm0
    .type latx_avx_single_vpor_observe_ymm0, @function
latx_avx_single_vpor_observe_ymm0:
    vpor ymm12, ymm0, ymmword ptr [rip + vpor_all_zeros]
    jmp .Lvpor_observe_common

    .globl latx_avx_single_vpor_observe_ymm15
    .type latx_avx_single_vpor_observe_ymm15, @function
latx_avx_single_vpor_observe_ymm15:
    vpor ymm12, ymm15, ymmword ptr [rip + vpor_all_zeros]
.Lvpor_observe_common:
    .globl latx_avx_single_vpor_observe_marker
latx_avx_single_vpor_observe_marker:
    vpor ymm15, ymm12, ymmword ptr [rip + vpor_all_zeros]
    vpor ymm14, ymm14, ymmword ptr [rip + vpor_all_zeros]
    ret

    .macro VPOR_FAULT_PREPARE
    VPOR_RESTORE_SEED
    vpor ymm15, ymm13, ymmword ptr [rip + vpor_all_zeros]
    .endm

    .globl latx_avx_single_vpor_fault_xmm
    .type latx_avx_single_vpor_fault_xmm, @function
latx_avx_single_vpor_fault_xmm:
    VPOR_FAULT_PREPARE
    vpor xmm15, xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vpor_fault_ymm
    .type latx_avx_single_vpor_fault_ymm, @function
latx_avx_single_vpor_fault_ymm:
    VPOR_FAULT_PREPARE
    vpor ymm15, ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vpor_fault_observe
    .type latx_avx_single_vpor_fault_observe, @function
latx_avx_single_vpor_fault_observe:
    jmp latx_avx_single_vpor_observe_ymm15

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vpor_seed_low:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vpor_all_zeros:
    .quad 0, 0, 0, 0
vpor_logic_mask:
    .quad 0x0f0f0f0f0f0f0f0f, 0xf0f0f0f0f0f0f0f0
vpor_masks:
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
vpor_rip_source:
    .quad 0, -1, 0x8000000000000000, 0x55aa55aa55aa55aa

    .section .data
    .balign 64
vpor_xrstor_area:
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
vpor_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
