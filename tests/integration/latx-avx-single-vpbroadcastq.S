/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VPBROADCASTQ_SAVE_RESULT target
    test rdi, rdi
    jz .Lvpbroadcastq_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vpbroadcastq_xsave_area + 160 + \target * 16], xmm14
    movdqu xmmword ptr [rip + vpbroadcastq_xsave_area + 576 + \target * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpbroadcastq_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpbroadcastq_xsave_area + 160 + \target * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + vpbroadcastq_xsave_area + 576 + \target * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvpbroadcastq_skip_save_\@:
    .endm

    .macro VPBROADCASTQ_RECORD_RESULT target
    call latx_avx_single_vpbroadcastq_observe_marker
    VPBROADCASTQ_SAVE_RESULT \target
    .endm

    .macro VPBROADCASTQ_SEED_TARGET target
    .if \target == 0
    vpbroadcastq ymm0, qword ptr [rip + vpbroadcastq_seed_ymm]
    .else
    vpbroadcastq ymm15, qword ptr [rip + vpbroadcastq_seed_ymm]
    .endif
    .endm

    .macro VPBROADCASTQ_PREPARE_ADDRESS address_variant
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

    .macro VPBROADCASTQ_XMM_MEM dest, address_variant
    .if \address_variant == 0
    vpbroadcastq \dest, qword ptr [rbx]
    .elseif \address_variant == 1
    vpbroadcastq \dest, qword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpbroadcastq \dest, qword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpbroadcastq \dest, qword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpbroadcastq \dest, qword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpbroadcastq \dest, qword ptr [rbx + 4088]
    .elseif \address_variant == 6
    vpbroadcastq \dest, qword ptr [rbx + 4092]
    .else
    vpbroadcastq \dest, qword ptr [rip + vpbroadcastq_rip_source]
    .endif
    .endm

    .macro VPBROADCASTQ_YMM_MEM dest, address_variant
    .if \address_variant == 0
    vpbroadcastq \dest, qword ptr [rbx]
    .elseif \address_variant == 1
    vpbroadcastq \dest, qword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vpbroadcastq \dest, qword ptr [rbx + 120]
    .elseif \address_variant == 3
    vpbroadcastq \dest, qword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vpbroadcastq \dest, qword ptr [rbx + 1]
    .elseif \address_variant == 5
    vpbroadcastq \dest, qword ptr [rbx + 4088]
    .elseif \address_variant == 6
    vpbroadcastq \dest, qword ptr [rbx + 4092]
    .else
    vpbroadcastq \dest, qword ptr [rip + vpbroadcastq_rip_source]
    .endif
    .endm

    .macro VPBROADCASTQ_XMM_MEM_CASE target, index
    VPBROADCASTQ_SEED_TARGET \target
    VPBROADCASTQ_PREPARE_ADDRESS (\index % 8)
    .if \target == 0
    VPBROADCASTQ_XMM_MEM xmm0, (\index % 8)
    .else
    VPBROADCASTQ_XMM_MEM xmm15, (\index % 8)
    .endif
    VPBROADCASTQ_RECORD_RESULT \target
    .endm

    .macro VPBROADCASTQ_YMM_MEM_CASE target, index
    VPBROADCASTQ_PREPARE_ADDRESS (\index % 8)
    .if \target == 0
    VPBROADCASTQ_YMM_MEM ymm0, (\index % 8)
    .else
    VPBROADCASTQ_YMM_MEM ymm15, (\index % 8)
    .endif
    VPBROADCASTQ_RECORD_RESULT \target
    .endm

    .macro VPBROADCASTQ_RUN_TARGET target
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vpbroadcastq_seed_low]
    vpbroadcastq ymm0, xmm0
    .else
    movdqu xmm15, xmmword ptr [rip + vpbroadcastq_seed_low]
    vpbroadcastq ymm15, xmm15
    .endif
    VPBROADCASTQ_RECORD_RESULT \target

    VPBROADCASTQ_SEED_TARGET \target
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vpbroadcastq_seed_low]
    vpbroadcastq xmm0, xmm0
    .else
    movdqu xmm15, xmmword ptr [rip + vpbroadcastq_seed_low]
    vpbroadcastq xmm15, xmm15
    .endif
    VPBROADCASTQ_RECORD_RESULT \target

    .if \target == 0
    movdqu xmm15, xmmword ptr [rip + vpbroadcastq_register_source]
    vpbroadcastq ymm0, xmm15
    .else
    movdqu xmm0, xmmword ptr [rip + vpbroadcastq_register_source]
    vpbroadcastq ymm15, xmm0
    .endif
    VPBROADCASTQ_RECORD_RESULT \target

    VPBROADCASTQ_SEED_TARGET \target
    .if \target == 0
    movdqu xmm15, xmmword ptr [rip + vpbroadcastq_register_source]
    vpbroadcastq xmm0, xmm15
    .else
    movdqu xmm0, xmmword ptr [rip + vpbroadcastq_register_source]
    vpbroadcastq xmm15, xmm0
    .endif
    VPBROADCASTQ_RECORD_RESULT \target

    .set vpbroadcastq_mem_index, 0
    .rept 8
        VPBROADCASTQ_XMM_MEM_CASE \target, vpbroadcastq_mem_index
        .set vpbroadcastq_mem_index, vpbroadcastq_mem_index + 1
    .endr
    .set vpbroadcastq_mem_index, 0
    .rept 8
        VPBROADCASTQ_YMM_MEM_CASE \target, vpbroadcastq_mem_index
        .set vpbroadcastq_mem_index, vpbroadcastq_mem_index + 1
    .endr
    .endm

    .globl latx_avx_single_vpbroadcastq_run
    .type latx_avx_single_vpbroadcastq_run, @function
latx_avx_single_vpbroadcastq_run:
    push rbx
    push r12
    push r13
    mov r13d, edx
    cmp r13d, 2
    je .Lvpbroadcastq_run_target15
    VPBROADCASTQ_RUN_TARGET 0
    cmp r13d, 1
    je .Lvpbroadcastq_run_done
.Lvpbroadcastq_run_target15:
    VPBROADCASTQ_RUN_TARGET 15
.Lvpbroadcastq_run_done:
    pop r13
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vpbroadcastq_observe_marker
    .type latx_avx_single_vpbroadcastq_observe_marker, @function
latx_avx_single_vpbroadcastq_observe_marker:
    vpbroadcastq ymm14, xmm14
    vpbroadcastq ymm14, xmm14
    ret

    .macro VPBROADCASTQ_FAULT_PREPARE
    vpbroadcastq ymm15, qword ptr [rip + vpbroadcastq_seed_ymm]
    .endm

    .globl latx_avx_single_vpbroadcastq_fault_xmm
    .type latx_avx_single_vpbroadcastq_fault_xmm, @function
latx_avx_single_vpbroadcastq_fault_xmm:
    VPBROADCASTQ_FAULT_PREPARE
    vpbroadcastq xmm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vpbroadcastq_fault_ymm
    .type latx_avx_single_vpbroadcastq_fault_ymm, @function
latx_avx_single_vpbroadcastq_fault_ymm:
    VPBROADCASTQ_FAULT_PREPARE
    vpbroadcastq ymm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vpbroadcastq_fault_observe
    .type latx_avx_single_vpbroadcastq_fault_observe, @function
latx_avx_single_vpbroadcastq_fault_observe:
    jmp latx_avx_single_vpbroadcastq_observe_marker

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vpbroadcastq_seed_low:
    .quad 0x0706050403020100, 0x8f8e8d8c8b8a8988
vpbroadcastq_seed_ymm:
    .quad 0x1122334455667788, 0x99aabbccddeeff00
    .quad 0x8000000000000001, 0x7ffffffffffffffe
vpbroadcastq_register_source:
    .quad 0x0123456789abcdef, 0xfedcba9876543210
    .quad 0x8000000000000000, 0x0000000000000001
vpbroadcastq_rip_source:
    .quad 0, -1, 0x8000000000000000, 1

    .section .bss
    .balign 64
vpbroadcastq_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
