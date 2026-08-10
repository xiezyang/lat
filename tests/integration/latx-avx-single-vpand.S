/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VPAND_RESTORE_SEED
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vpand_xrstor_area]
    movdqu xmm15, xmmword ptr [rip + vpand_seed_low]
    vpand ymm13, ymm15, ymmword ptr [rip + vpand_all_ones]
    .endm

    .macro VPAND_SETUP_REGS index
    vpand ymm0, ymm13, ymmword ptr [rip + vpand_masks + ((\index % 6) * 32)]
    vpand ymm1, ymm13, ymmword ptr [rip + vpand_masks + (((\index + 1) % 6) * 32)]
    vpand ymm2, ymm13, ymmword ptr [rip + vpand_masks + (((\index + 2) % 6) * 32)]
    vpand ymm15, ymm13, ymmword ptr [rip + vpand_masks + (((\index + 3) % 6) * 32)]
    .endm

    .macro VPAND_SAVE_RESULT
    test rdi, rdi
    jz .Lvpand_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vpand_xsave_area + 160 + 15 * 16], xmm14
    movdqu xmmword ptr [rip + vpand_xsave_area + 576 + 15 * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpand_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpand_xsave_area + 160 + 15 * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + vpand_xsave_area + 576 + 15 * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvpand_skip_save_\@:
    .endm

    .macro VPAND_REG_CASE width, variant
    VPAND_SETUP_REGS \variant
    .if \width == 128
        .if \variant == 0
        vpand xmm0, xmm1, xmm2
        .elseif \variant == 1
        vpand xmm0, xmm0, xmm2
        .elseif \variant == 2
        vpand xmm0, xmm1, xmm0
        .elseif \variant == 3
        vpand xmm0, xmm1, xmm1
        .elseif \variant == 4
        vpand xmm0, xmm0, xmm0
        .elseif \variant == 5
        vpand xmm15, xmm1, xmm2
        .elseif \variant == 6
        vpand xmm15, xmm15, xmm2
        .elseif \variant == 7
        vpand xmm15, xmm1, xmm15
        .elseif \variant == 8
        vpand xmm15, xmm1, xmm1
        .else
        vpand xmm15, xmm15, xmm15
        .endif
    .else
        .if \variant == 0
        vpand ymm0, ymm1, ymm2
        .elseif \variant == 1
        vpand ymm0, ymm0, ymm2
        .elseif \variant == 2
        vpand ymm0, ymm1, ymm0
        .elseif \variant == 3
        vpand ymm0, ymm1, ymm1
        .elseif \variant == 4
        vpand ymm0, ymm0, ymm0
        .elseif \variant == 5
        vpand ymm15, ymm1, ymm2
        .elseif \variant == 6
        vpand ymm15, ymm15, ymm2
        .elseif \variant == 7
        vpand ymm15, ymm1, ymm15
        .elseif \variant == 8
        vpand ymm15, ymm1, ymm1
        .else
        vpand ymm15, ymm15, ymm15
        .endif
    .endif
    .if \variant < 5
    call latx_avx_single_vpand_observe_ymm0
    .else
    call latx_avx_single_vpand_observe_ymm15
    .endif
    VPAND_SAVE_RESULT
    .endm

    .macro VPAND_PREPARE_ADDRESS width, address_variant
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

    .macro VPAND_XMM_MEM dest, src, address_variant
    .if \address_variant == 0
    vpand \dest, \src, xmmword ptr [rbx]
    .elseif \address_variant == 1
    vpand \dest, \src, xmmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpand \dest, \src, xmmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpand \dest, \src, xmmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpand \dest, \src, xmmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpand \dest, \src, xmmword ptr [rbx + 4080]
    .elseif \address_variant == 6
    vpand \dest, \src, xmmword ptr [rbx + 4090]
    .else
    vpand \dest, \src, xmmword ptr [rip + vpand_rip_source]
    .endif
    .endm

    .macro VPAND_YMM_MEM dest, src, address_variant
    .if \address_variant == 0
    vpand \dest, \src, ymmword ptr [rbx]
    .elseif \address_variant == 1
    vpand \dest, \src, ymmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpand \dest, \src, ymmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpand \dest, \src, ymmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpand \dest, \src, ymmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpand \dest, \src, ymmword ptr [rbx + 4064]
    .elseif \address_variant == 6
    vpand \dest, \src, ymmword ptr [rbx + 4080]
    .else
    vpand \dest, \src, ymmword ptr [rip + vpand_rip_source]
    .endif
    .endm

    .macro VPAND_MEM_CASE width, index
    VPAND_SETUP_REGS \index
    VPAND_PREPARE_ADDRESS \width, (\index % 8)
    .if \width == 128
        .if (\index % 4) == 0
        VPAND_XMM_MEM xmm0, xmm1, (\index % 8)
        .elseif (\index % 4) == 1
        VPAND_XMM_MEM xmm0, xmm0, (\index % 8)
        .elseif (\index % 4) == 2
        VPAND_XMM_MEM xmm15, xmm1, (\index % 8)
        .else
        VPAND_XMM_MEM xmm15, xmm15, (\index % 8)
        .endif
    .else
        .if (\index % 4) == 0
        VPAND_YMM_MEM ymm0, ymm1, (\index % 8)
        .elseif (\index % 4) == 1
        VPAND_YMM_MEM ymm0, ymm0, (\index % 8)
        .elseif (\index % 4) == 2
        VPAND_YMM_MEM ymm15, ymm1, (\index % 8)
        .else
        VPAND_YMM_MEM ymm15, ymm15, (\index % 8)
        .endif
    .endif
    .if (\index % 4) < 2
    call latx_avx_single_vpand_observe_ymm0
    .else
    call latx_avx_single_vpand_observe_ymm15
    .endif
    VPAND_SAVE_RESULT
    .endm

    .globl latx_avx_single_vpand_run
    .type latx_avx_single_vpand_run, @function
latx_avx_single_vpand_run:
    push rbx
    push r12
    VPAND_RESTORE_SEED

    .set vpand_reg_variant, 0
    .rept 10
        VPAND_REG_CASE 128, vpand_reg_variant
        .set vpand_reg_variant, vpand_reg_variant + 1
    .endr
    .set vpand_reg_variant, 0
    .rept 10
        VPAND_REG_CASE 256, vpand_reg_variant
        .set vpand_reg_variant, vpand_reg_variant + 1
    .endr

    .set vpand_mem_index, 0
    .rept 32
        VPAND_MEM_CASE 128, vpand_mem_index
        .set vpand_mem_index, vpand_mem_index + 1
    .endr
    .set vpand_mem_index, 0
    .rept 32
        VPAND_MEM_CASE 256, vpand_mem_index
        .set vpand_mem_index, vpand_mem_index + 1
    .endr

    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vpand_logic
    .type latx_avx_single_vpand_logic, @function
latx_avx_single_vpand_logic:
    movdqu xmm0, xmmword ptr [rip + vpand_seed_low]
    movdqu xmm1, xmmword ptr [rip + vpand_logic_mask]
    vpand xmm2, xmm0, xmm1
    movdqu xmmword ptr [rdi], xmm2
    movdqu xmm3, xmmword ptr [rip + vpand_seed_low]
    vpand xmm4, xmm3, xmm1
    movdqu xmmword ptr [rdi + 16], xmm4
    vpand ymm6, ymm6, ymm6
    vpand ymm6, ymm6, ymmword ptr [rip + vpand_logic_mask]
    ret

    .globl latx_avx_single_vpand_observe_ymm0
    .type latx_avx_single_vpand_observe_ymm0, @function
latx_avx_single_vpand_observe_ymm0:
    vpand ymm12, ymm0, ymmword ptr [rip + vpand_all_ones]
    jmp .Lvpand_observe_common

    .globl latx_avx_single_vpand_observe_ymm15
    .type latx_avx_single_vpand_observe_ymm15, @function
latx_avx_single_vpand_observe_ymm15:
    vpand ymm12, ymm15, ymmword ptr [rip + vpand_all_ones]
.Lvpand_observe_common:
    .globl latx_avx_single_vpand_observe_marker
latx_avx_single_vpand_observe_marker:
    vpand ymm15, ymm12, ymmword ptr [rip + vpand_all_ones]
    vpand ymm14, ymm14, ymmword ptr [rip + vpand_all_ones]
    ret

    .macro VPAND_FAULT_PREPARE
    VPAND_RESTORE_SEED
    vpand ymm15, ymm13, ymmword ptr [rip + vpand_all_ones]
    .endm

    .globl latx_avx_single_vpand_fault_xmm
    .type latx_avx_single_vpand_fault_xmm, @function
latx_avx_single_vpand_fault_xmm:
    VPAND_FAULT_PREPARE
    vpand xmm15, xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vpand_fault_ymm
    .type latx_avx_single_vpand_fault_ymm, @function
latx_avx_single_vpand_fault_ymm:
    VPAND_FAULT_PREPARE
    vpand ymm15, ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vpand_fault_observe
    .type latx_avx_single_vpand_fault_observe, @function
latx_avx_single_vpand_fault_observe:
    jmp latx_avx_single_vpand_observe_ymm15

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vpand_seed_low:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vpand_all_ones:
    .quad -1, -1, -1, -1
vpand_logic_mask:
    .quad 0x0f0f0f0f0f0f0f0f, 0xf0f0f0f0f0f0f0f0
vpand_masks:
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
vpand_rip_source:
    .quad 0, -1, 0x8000000000000000, 0x55aa55aa55aa55aa

    .section .data
    .balign 64
vpand_xrstor_area:
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
vpand_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
