/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VMOVDDUP_RESTORE_SEED target
    test rdi, rdi
    jz .Lvmovddup_skip_restore_\@
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vmovddup_xrstor_area]
.Lvmovddup_skip_restore_\@:
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vmovddup_seed_low]
    .else
    movdqu xmm15, xmmword ptr [rip + vmovddup_seed_low]
    .endif
    .endm

    .macro VMOVDDUP_SAVE_RESULT target
    test rdi, rdi
    jz .Lvmovddup_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vmovddup_xsave_area + 160 + \target * 16], xmm14
    movdqu xmmword ptr [rip + vmovddup_xsave_area + 576 + \target * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vmovddup_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vmovddup_xsave_area + 160 + \target * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + vmovddup_xsave_area + 576 + \target * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvmovddup_skip_save_\@:
    .endm

    .macro VMOVDDUP_RECORD_RESULT target
    call latx_avx_single_vmovddup_observe_marker
    VMOVDDUP_SAVE_RESULT \target
    .endm

    .macro VMOVDDUP_SEED_TARGET target
    .if \target == 0
    vmovddup ymm0, ymmword ptr [rip + vmovddup_seed_ymm]
    .else
    vmovddup ymm15, ymmword ptr [rip + vmovddup_seed_ymm]
    .endif
    .endm

    .macro VMOVDDUP_PREPARE_ADDRESS address_variant
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

    .macro VMOVDDUP_XMM_MEM dest, address_variant
    .if \address_variant == 0
    vmovddup \dest, qword ptr [rbx]
    .elseif \address_variant == 1
    vmovddup \dest, qword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vmovddup \dest, qword ptr [rbx + 120]
    .elseif \address_variant == 3
    vmovddup \dest, qword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vmovddup \dest, qword ptr [rbx + 1]
    .elseif \address_variant == 5
    vmovddup \dest, qword ptr [rbx + 4088]
    .elseif \address_variant == 6
    vmovddup \dest, qword ptr [rbx + 4092]
    .else
    vmovddup \dest, qword ptr [rip + vmovddup_rip_source]
    .endif
    .endm

    .macro VMOVDDUP_YMM_MEM dest, address_variant
    .if \address_variant == 0
    vmovddup \dest, ymmword ptr [rbx]
    .elseif \address_variant == 1
    vmovddup \dest, ymmword ptr [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
    vmovddup \dest, ymmword ptr [rbx + 120]
    .elseif \address_variant == 3
    vmovddup \dest, ymmword ptr [rsi + 25600]
    .elseif \address_variant == 4
    vmovddup \dest, ymmword ptr [rbx + 1]
    .elseif \address_variant == 5
    vmovddup \dest, ymmword ptr [rbx + 4064]
    .elseif \address_variant == 6
    vmovddup \dest, ymmword ptr [rbx + 4080]
    .else
    vmovddup \dest, ymmword ptr [rip + vmovddup_rip_source]
    .endif
    .endm

    .macro VMOVDDUP_XMM_MEM_CASE target, index
    VMOVDDUP_SEED_TARGET \target
    VMOVDDUP_PREPARE_ADDRESS (\index % 8)
    .if \target == 0
    VMOVDDUP_XMM_MEM xmm0, (\index % 8)
    .else
    VMOVDDUP_XMM_MEM xmm15, (\index % 8)
    .endif
    VMOVDDUP_RECORD_RESULT \target
    .endm

    .macro VMOVDDUP_YMM_MEM_CASE target, index
    VMOVDDUP_PREPARE_ADDRESS (\index % 8)
    .if \target == 0
    VMOVDDUP_YMM_MEM ymm0, (\index % 8)
    .else
    VMOVDDUP_YMM_MEM ymm15, (\index % 8)
    .endif
    VMOVDDUP_RECORD_RESULT \target
    .endm

    .macro VMOVDDUP_RUN_TARGET target
    VMOVDDUP_RESTORE_SEED \target

    /* First AVX hit consumes the injected distinct high qwords. */
    .if \target == 0
    vmovddup ymm0, ymm0
    .else
    vmovddup ymm15, ymm15
    .endif
    VMOVDDUP_RECORD_RESULT \target

    VMOVDDUP_SEED_TARGET \target
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vmovddup_seed_low]
    vmovddup xmm0, xmm0
    .else
    movdqu xmm15, xmmword ptr [rip + vmovddup_seed_low]
    vmovddup xmm15, xmm15
    .endif
    VMOVDDUP_RECORD_RESULT \target

    vmovddup ymm1, ymmword ptr [rip + vmovddup_register_source]
    .if \target == 0
    vmovddup ymm0, ymm1
    .else
    vmovddup ymm15, ymm1
    .endif
    VMOVDDUP_RECORD_RESULT \target

    VMOVDDUP_SEED_TARGET \target
    movdqu xmm1, xmmword ptr [rip + vmovddup_register_source]
    .if \target == 0
    vmovddup xmm0, xmm1
    .else
    vmovddup xmm15, xmm1
    .endif
    VMOVDDUP_RECORD_RESULT \target

    .set vmovddup_mem_index, 0
    .rept 8
        VMOVDDUP_XMM_MEM_CASE \target, vmovddup_mem_index
        .set vmovddup_mem_index, vmovddup_mem_index + 1
    .endr
    .set vmovddup_mem_index, 0
    .rept 8
        VMOVDDUP_YMM_MEM_CASE \target, vmovddup_mem_index
        .set vmovddup_mem_index, vmovddup_mem_index + 1
    .endr
    .endm

    .globl latx_avx_single_vmovddup_run
    .type latx_avx_single_vmovddup_run, @function
latx_avx_single_vmovddup_run:
    push rbx
    push r12
    push r13
    mov r13d, edx
    cmp r13d, 2
    je .Lvmovddup_run_target15
    VMOVDDUP_RUN_TARGET 0
    cmp r13d, 1
    je .Lvmovddup_run_done
.Lvmovddup_run_target15:
    VMOVDDUP_RUN_TARGET 15
.Lvmovddup_run_done:
    pop r13
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vmovddup_observe_marker
    .type latx_avx_single_vmovddup_observe_marker, @function
latx_avx_single_vmovddup_observe_marker:
    vmovddup ymm14, ymm14
    vmovddup ymm14, ymm14
    ret

    .macro VMOVDDUP_FAULT_PREPARE
    vmovddup ymm15, ymmword ptr [rip + vmovddup_seed_ymm]
    .endm

    .globl latx_avx_single_vmovddup_fault_xmm
    .type latx_avx_single_vmovddup_fault_xmm, @function
latx_avx_single_vmovddup_fault_xmm:
    VMOVDDUP_FAULT_PREPARE
    vmovddup xmm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vmovddup_fault_ymm
    .type latx_avx_single_vmovddup_fault_ymm, @function
latx_avx_single_vmovddup_fault_ymm:
    VMOVDDUP_FAULT_PREPARE
    vmovddup ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovddup_fault_observe
    .type latx_avx_single_vmovddup_fault_observe, @function
latx_avx_single_vmovddup_fault_observe:
    jmp latx_avx_single_vmovddup_observe_marker

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vmovddup_seed_low:
    .quad 0x0706050403020100, 0x8f8e8d8c8b8a8988
vmovddup_seed_ymm:
    .quad 0x7ff8000000000001, 0x1122334455667788
    .quad 0x7ff0000000000001, 0x99aabbccddeeff00
vmovddup_register_source:
    .quad 0x0123456789abcdef, 0xfedcba9876543210
    .quad 0x8000000000000000, 0x0000000000000001
vmovddup_rip_source:
    .quad 0, -1, 0x8000000000000000, 1

    .section .data
    .balign 64
vmovddup_xrstor_area:
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
vmovddup_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
