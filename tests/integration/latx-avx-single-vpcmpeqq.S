/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VPCMPEQQ_RESTORE_SEED index
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vpcmpeqq_xrstor_area]
    .if \index == 0
    movdqu xmm0, xmmword ptr [rip + vpcmpeqq_seed_low]
    vpcmpeqq ymm13, ymm0, ymmword ptr [rip + vpcmpeqq_seed_compare]
    .else
    movdqu xmm15, xmmword ptr [rip + vpcmpeqq_seed_low]
    vpcmpeqq ymm13, ymm15, ymmword ptr [rip + vpcmpeqq_seed_compare]
    .endif
    .endm

    .macro VPCMPEQQ_SETUP_REGS index
    vpcmpeqq ymm0, ymm13, ymmword ptr [rip + vpcmpeqq_masks + ((\index % 6) * 32)]
    vpcmpeqq ymm1, ymm13, ymmword ptr [rip + vpcmpeqq_masks + (((\index + 1) % 6) * 32)]
    vpcmpeqq ymm2, ymm13, ymmword ptr [rip + vpcmpeqq_masks + (((\index + 2) % 6) * 32)]
    vpcmpeqq ymm15, ymm13, ymmword ptr [rip + vpcmpeqq_masks + (((\index + 3) % 6) * 32)]
    .endm

    .macro VPCMPEQQ_SAVE_RESULT index
    test rdi, rdi
    jz .Lvpcmpeqq_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vpcmpeqq_xsave_area + 160 + \index * 16], xmm14
    movdqu xmmword ptr [rip + vpcmpeqq_xsave_area + 576 + \index * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpcmpeqq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpcmpeqq_xsave_area + 160 + \index * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + vpcmpeqq_xsave_area + 576 + \index * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvpcmpeqq_skip_save_\@:
    .endm

    .macro VPCMPEQQ_RECORD_RESULT index
    call latx_avx_single_vpcmpeqq_observe_marker
    VPCMPEQQ_SAVE_RESULT \index
    .endm

    .macro VPCMPEQQ_REG_CASE width, target, variant
    VPCMPEQQ_SETUP_REGS (\variant + \target)
    .if \width == 128
        .if \target == 0
            .if \variant == 0
            vpcmpeqq xmm0, xmm1, xmm2
            .elseif \variant == 1
            vpcmpeqq xmm0, xmm0, xmm2
            .elseif \variant == 2
            vpcmpeqq xmm0, xmm1, xmm0
            .elseif \variant == 3
            vpcmpeqq xmm0, xmm1, xmm1
            .else
            vpcmpeqq xmm0, xmm0, xmm0
            .endif
        .else
            .if \variant == 0
            vpcmpeqq xmm15, xmm1, xmm2
            .elseif \variant == 1
            vpcmpeqq xmm15, xmm15, xmm2
            .elseif \variant == 2
            vpcmpeqq xmm15, xmm1, xmm15
            .elseif \variant == 3
            vpcmpeqq xmm15, xmm1, xmm1
            .else
            vpcmpeqq xmm15, xmm15, xmm15
            .endif
        .endif
    .else
        .if \target == 0
            .if \variant == 0
            vpcmpeqq ymm0, ymm1, ymm2
            .elseif \variant == 1
            vpcmpeqq ymm0, ymm0, ymm2
            .elseif \variant == 2
            vpcmpeqq ymm0, ymm1, ymm0
            .elseif \variant == 3
            vpcmpeqq ymm0, ymm1, ymm1
            .else
            vpcmpeqq ymm0, ymm0, ymm0
            .endif
        .else
            .if \variant == 0
            vpcmpeqq ymm15, ymm1, ymm2
            .elseif \variant == 1
            vpcmpeqq ymm15, ymm15, ymm2
            .elseif \variant == 2
            vpcmpeqq ymm15, ymm1, ymm15
            .elseif \variant == 3
            vpcmpeqq ymm15, ymm1, ymm1
            .else
            vpcmpeqq ymm15, ymm15, ymm15
            .endif
        .endif
    .endif
    VPCMPEQQ_RECORD_RESULT \target
    .endm

    .macro VPCMPEQQ_PREPARE_ADDRESS address_variant
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

    .macro VPCMPEQQ_XMM_MEM dest, src, address_variant
    .if \address_variant == 0
    vpcmpeqq \dest, \src, xmmword ptr [rbx]
    .elseif \address_variant == 1
    vpcmpeqq \dest, \src, xmmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpcmpeqq \dest, \src, xmmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpcmpeqq \dest, \src, xmmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpcmpeqq \dest, \src, xmmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpcmpeqq \dest, \src, xmmword ptr [rbx + 4080]
    .elseif \address_variant == 6
    vpcmpeqq \dest, \src, xmmword ptr [rbx + 4090]
    .else
    vpcmpeqq \dest, \src, xmmword ptr [rip + vpcmpeqq_rip_source]
    .endif
    .endm

    .macro VPCMPEQQ_YMM_MEM dest, src, address_variant
    .if \address_variant == 0
    vpcmpeqq \dest, \src, ymmword ptr [rbx]
    .elseif \address_variant == 1
    vpcmpeqq \dest, \src, ymmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpcmpeqq \dest, \src, ymmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpcmpeqq \dest, \src, ymmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpcmpeqq \dest, \src, ymmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpcmpeqq \dest, \src, ymmword ptr [rbx + 4064]
    .elseif \address_variant == 6
    vpcmpeqq \dest, \src, ymmword ptr [rbx + 4080]
    .else
    vpcmpeqq \dest, \src, ymmword ptr [rip + vpcmpeqq_rip_source]
    .endif
    .endm

    .macro VPCMPEQQ_MEM_CASE width, target, index
    VPCMPEQQ_SETUP_REGS (\index + \target)
    VPCMPEQQ_PREPARE_ADDRESS (\index % 8)
    .if \width == 128
        .if \target == 0
            .if (\index % 2) == 0
            VPCMPEQQ_XMM_MEM xmm0, xmm1, (\index % 8)
            .else
            VPCMPEQQ_XMM_MEM xmm0, xmm0, (\index % 8)
            .endif
        .else
            .if (\index % 2) == 0
            VPCMPEQQ_XMM_MEM xmm15, xmm1, (\index % 8)
            .else
            VPCMPEQQ_XMM_MEM xmm15, xmm15, (\index % 8)
            .endif
        .endif
    .else
        .if \target == 0
            .if (\index % 2) == 0
            VPCMPEQQ_YMM_MEM ymm0, ymm1, (\index % 8)
            .else
            VPCMPEQQ_YMM_MEM ymm0, ymm0, (\index % 8)
            .endif
        .else
            .if (\index % 2) == 0
            VPCMPEQQ_YMM_MEM ymm15, ymm1, (\index % 8)
            .else
            VPCMPEQQ_YMM_MEM ymm15, ymm15, (\index % 8)
            .endif
        .endif
    .endif
    VPCMPEQQ_RECORD_RESULT \target
    .endm

    .macro VPCMPEQQ_RUN_TARGET target
    .set vpcmpeqq_reg_variant, 0
    .rept 5
        VPCMPEQQ_REG_CASE 128, \target, vpcmpeqq_reg_variant
        .set vpcmpeqq_reg_variant, vpcmpeqq_reg_variant + 1
    .endr
    .set vpcmpeqq_reg_variant, 0
    .rept 5
        VPCMPEQQ_REG_CASE 256, \target, vpcmpeqq_reg_variant
        .set vpcmpeqq_reg_variant, vpcmpeqq_reg_variant + 1
    .endr

    .set vpcmpeqq_mem_index, 0
    .rept 16
        VPCMPEQQ_MEM_CASE 128, \target, vpcmpeqq_mem_index
        .set vpcmpeqq_mem_index, vpcmpeqq_mem_index + 1
    .endr
    .set vpcmpeqq_mem_index, 0
    .rept 16
        VPCMPEQQ_MEM_CASE 256, \target, vpcmpeqq_mem_index
        .set vpcmpeqq_mem_index, vpcmpeqq_mem_index + 1
    .endr
    .endm

    .globl latx_avx_single_vpcmpeqq_run
    .type latx_avx_single_vpcmpeqq_run, @function
latx_avx_single_vpcmpeqq_run:
    push rbx
    push r12
    push r13
    mov r13d, edx
    cmp r13d, 2
    je .Lvpcmpeqq_run_target15
    VPCMPEQQ_RESTORE_SEED 0
    VPCMPEQQ_RUN_TARGET 0
    cmp r13d, 1
    je .Lvpcmpeqq_run_done
.Lvpcmpeqq_run_target15:
    VPCMPEQQ_RESTORE_SEED 15
    VPCMPEQQ_RUN_TARGET 15
.Lvpcmpeqq_run_done:
    pop r13
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vpcmpeqq_observe_marker
    .type latx_avx_single_vpcmpeqq_observe_marker, @function
latx_avx_single_vpcmpeqq_observe_marker:
    vpcmpeqq ymm14, ymm14, ymmword ptr [rip + vpcmpeqq_all_ones]
    vpcmpeqq ymm14, ymm14, ymmword ptr [rip + vpcmpeqq_all_ones]
    ret

    .macro VPCMPEQQ_FAULT_PREPARE
    VPCMPEQQ_RESTORE_SEED 15
    vpcmpeqq ymm15, ymm13, ymmword ptr [rip + vpcmpeqq_all_ones]
    .endm

    .globl latx_avx_single_vpcmpeqq_fault_xmm
    .type latx_avx_single_vpcmpeqq_fault_xmm, @function
latx_avx_single_vpcmpeqq_fault_xmm:
    VPCMPEQQ_FAULT_PREPARE
    vpcmpeqq xmm15, xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vpcmpeqq_fault_ymm
    .type latx_avx_single_vpcmpeqq_fault_ymm, @function
latx_avx_single_vpcmpeqq_fault_ymm:
    VPCMPEQQ_FAULT_PREPARE
    vpcmpeqq ymm15, ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vpcmpeqq_fault_observe
    .type latx_avx_single_vpcmpeqq_fault_observe, @function
latx_avx_single_vpcmpeqq_fault_observe:
    jmp latx_avx_single_vpcmpeqq_observe_marker

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vpcmpeqq_seed_low:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vpcmpeqq_seed_compare:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f851
    .quad 0x1122334455667788, 0x99aabbccddeeff01
vpcmpeqq_all_ones:
    .quad -1, -1, -1, -1
vpcmpeqq_masks:
    .quad -1, 0, -1, 0
    .quad 0, -1, 0, -1
    .quad 0, 0, 0, 0
    .quad -1, -1, -1, -1
    .quad -1, 0, 0, -1
    .quad 0, 0, -1, -1
vpcmpeqq_rip_source:
    .quad 0x00000000ffffffff, 0xffffffff00000000, 0, -1

    .section .data
    .balign 64
vpcmpeqq_xrstor_area:
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
vpcmpeqq_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
