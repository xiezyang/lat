/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VMOVSD_RESTORE_YMM_SEED
    test rdi, rdi
    jz .Lvmovsd_skip_restore_\@
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vmovsd_xrstor_area]
.Lvmovsd_skip_restore_\@:
    .endm

    .macro VMOVSD_SAVE_RESULT target, snapshot
    test rdi, rdi
    jz .Lvmovsd_skip_save_\@
    pxor xmm12, xmm12
    movdqu xmmword ptr [rip + vmovsd_xsave_area + 160 + \target * 16], \
        xmm12
    movdqu xmmword ptr [rip + vmovsd_xsave_area + 576 + \target * 16], \
        xmm12
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vmovsd_xsave_area]
    movdqu xmm12, \
        xmmword ptr [rip + vmovsd_xsave_area + 160 + \target * 16]
    movdqu xmmword ptr [rdi], xmm12
    movdqu xmm12, \
        xmmword ptr [rip + vmovsd_xsave_area + 576 + \target * 16]
    movdqu xmmword ptr [rdi + 16], xmm12
    .if \snapshot
    movdqu xmm12, xmmword ptr [r14]
    movdqu xmmword ptr [rdi + 32], xmm12
    movdqu xmm12, xmmword ptr [r14 + 16]
    movdqu xmmword ptr [rdi + 48], xmm12
    .else
    pxor xmm12, xmm12
    movdqu xmmword ptr [rdi + 32], xmm12
    movdqu xmmword ptr [rdi + 48], xmm12
    .endif
    add rdi, 64
.Lvmovsd_skip_save_\@:
    .endm

    .macro VMOVSD_RECORD_RESULT target, snapshot=0
    call latx_avx_single_vmovsd_observe_marker
    VMOVSD_SAVE_RESULT \target, \snapshot
    .endm

    .macro VMOVSD_SEED_TARGET target, source
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + \source]
    .else
    movdqu xmm15, xmmword ptr [rip + \source]
    .endif
    .endm

    .macro VMOVSD_STORE_CASE target, address_variant
    VMOVSD_SEED_TARGET \target, vmovsd_store_source
    .if \address_variant == 0
        .if \target == 0
        vmovsd qword ptr [r13], xmm0
        .else
        vmovsd qword ptr [r13], xmm15
        .endif
        lea r14, [r13]
    .elseif \address_variant == 1
        lea rbx, [r13 + 8192]
        mov r12, 3
        .if \target == 0
        vmovsd qword ptr [rbx + r12 * 2 + 7], xmm0
        .else
        vmovsd qword ptr [rbx + r12 * 2 + 7], xmm15
        .endif
        lea r14, [rbx + r12 * 2 + 7]
    .elseif \address_variant == 2
        lea rbx, [r13 + 16384]
        .if \target == 0
        vmovsd qword ptr [rbx + 120], xmm0
        .else
        vmovsd qword ptr [rbx + 120], xmm15
        .endif
        lea r14, [rbx + 120]
    .elseif \address_variant == 3
        .if \target == 0
        vmovsd qword ptr [r13 + 25600], xmm0
        .else
        vmovsd qword ptr [r13 + 25600], xmm15
        .endif
        lea r14, [r13 + 25600]
    .elseif \address_variant == 4
        .if \target == 0
        vmovsd qword ptr [rip + latx_avx_vmovsd_rip_store], xmm0
        .else
        vmovsd qword ptr [rip + latx_avx_vmovsd_rip_store], xmm15
        .endif
        lea r14, [rip + latx_avx_vmovsd_rip_store]
    .elseif \address_variant == 5
        lea rbx, [r13 + 32768]
        .if \target == 0
        vmovsd qword ptr [rbx + 1], xmm0
        .else
        vmovsd qword ptr [rbx + 1], xmm15
        .endif
        lea r14, [rbx + 1]
    .elseif \address_variant == 6
        lea rbx, [r13 + 40960]
        .if \target == 0
        vmovsd qword ptr [rbx + 4088], xmm0
        .else
        vmovsd qword ptr [rbx + 4088], xmm15
        .endif
        lea r14, [rbx + 4064]
    .else
        lea rbx, [r13 + 49152]
        .if \target == 0
        vmovsd qword ptr [rbx + 4092], xmm0
        .else
        vmovsd qword ptr [rbx + 4092], xmm15
        .endif
        lea r14, [rbx + 4092]
    .endif
    VMOVSD_RECORD_RESULT \target, 1
    .endm

    .macro VMOVSD_LOAD_CASE target, address_variant
    VMOVSD_SEED_TARGET \target, vmovsd_target_seed
    .if \address_variant == 0
        lea rbx, [rsi]
        .if \target == 0
        vmovsd xmm0, qword ptr [rbx]
        .else
        vmovsd xmm15, qword ptr [rbx]
        .endif
    .elseif \address_variant == 1
        lea rbx, [rsi + 8192]
        mov r12, 3
        .if \target == 0
        vmovsd xmm0, qword ptr [rbx + r12 * 2 + 7]
        .else
        vmovsd xmm15, qword ptr [rbx + r12 * 2 + 7]
        .endif
    .elseif \address_variant == 2
        lea rbx, [rsi + 16384]
        .if \target == 0
        vmovsd xmm0, qword ptr [rbx + 120]
        .else
        vmovsd xmm15, qword ptr [rbx + 120]
        .endif
    .elseif \address_variant == 3
        .if \target == 0
        vmovsd xmm0, qword ptr [rsi + 25600]
        .else
        vmovsd xmm15, qword ptr [rsi + 25600]
        .endif
    .elseif \address_variant == 4
        .if \target == 0
        vmovsd xmm0, qword ptr [rip + latx_avx_vmovsd_rip_source]
        .else
        vmovsd xmm15, qword ptr [rip + latx_avx_vmovsd_rip_source]
        .endif
    .elseif \address_variant == 5
        lea rbx, [rsi + 32768]
        .if \target == 0
        vmovsd xmm0, qword ptr [rbx + 1]
        .else
        vmovsd xmm15, qword ptr [rbx + 1]
        .endif
    .elseif \address_variant == 6
        lea rbx, [rsi + 40960]
        .if \target == 0
        vmovsd xmm0, qword ptr [rbx + 4088]
        .else
        vmovsd xmm15, qword ptr [rbx + 4088]
        .endif
    .else
        lea rbx, [rsi + 49152]
        .if \target == 0
        vmovsd xmm0, qword ptr [rbx + 4092]
        .else
        vmovsd xmm15, qword ptr [rbx + 4092]
        .endif
    .endif
    VMOVSD_RECORD_RESULT \target
    .endm

    .macro VMOVSD_RUN_STORES target
    VMOVSD_RESTORE_YMM_SEED
    .set vmovsd_store_index, 0
    .rept 8
        VMOVSD_STORE_CASE \target, vmovsd_store_index
        .set vmovsd_store_index, vmovsd_store_index + 1
    .endr
    .endm

    .macro VMOVSD_RUN_LOADS target
    VMOVSD_RESTORE_YMM_SEED
    .set vmovsd_load_index, 0
    .rept 8
        VMOVSD_LOAD_CASE \target, vmovsd_load_index
        .set vmovsd_load_index, vmovsd_load_index + 1
    .endr
    .endm

    .macro VMOVSD_RUN_REGISTERS target
    VMOVSD_RESTORE_YMM_SEED
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vmovsd_target_seed]
    vmovsd xmm0, xmm0, xmm0
    VMOVSD_RECORD_RESULT 0

    movdqu xmm0, xmmword ptr [rip + vmovsd_src1]
    movdqu xmm1, xmmword ptr [rip + vmovsd_src2]
    vmovsd xmm0, xmm0, xmm1
    VMOVSD_RECORD_RESULT 0

    movdqu xmm0, xmmword ptr [rip + vmovsd_src2]
    movdqu xmm1, xmmword ptr [rip + vmovsd_src1]
    vmovsd xmm0, xmm1, xmm0
    VMOVSD_RECORD_RESULT 0

    movdqu xmm0, xmmword ptr [rip + vmovsd_target_seed]
    movdqu xmm1, xmmword ptr [rip + vmovsd_src1]
    vmovsd xmm0, xmm1, xmm1
    VMOVSD_RECORD_RESULT 0

    movdqu xmm0, xmmword ptr [rip + vmovsd_target_seed]
    movdqu xmm1, xmmword ptr [rip + vmovsd_src1]
    movdqu xmm2, xmmword ptr [rip + vmovsd_src2]
    vmovsd xmm0, xmm1, xmm2
    VMOVSD_RECORD_RESULT 0
    .else
    movdqu xmm15, xmmword ptr [rip + vmovsd_target_seed]
    vmovsd xmm15, xmm15, xmm15
    VMOVSD_RECORD_RESULT 15

    movdqu xmm15, xmmword ptr [rip + vmovsd_src1]
    movdqu xmm14, xmmword ptr [rip + vmovsd_src2]
    vmovsd xmm15, xmm15, xmm14
    VMOVSD_RECORD_RESULT 15

    movdqu xmm15, xmmword ptr [rip + vmovsd_src2]
    movdqu xmm14, xmmword ptr [rip + vmovsd_src1]
    vmovsd xmm15, xmm14, xmm15
    VMOVSD_RECORD_RESULT 15

    movdqu xmm15, xmmword ptr [rip + vmovsd_target_seed]
    movdqu xmm14, xmmword ptr [rip + vmovsd_src1]
    vmovsd xmm15, xmm14, xmm14
    VMOVSD_RECORD_RESULT 15

    movdqu xmm15, xmmword ptr [rip + vmovsd_target_seed]
    movdqu xmm13, xmmword ptr [rip + vmovsd_src1]
    movdqu xmm14, xmmword ptr [rip + vmovsd_src2]
    vmovsd xmm15, xmm13, xmm14
    VMOVSD_RECORD_RESULT 15
    .endif
    .endm

    .globl latx_avx_single_vmovsd_run
    .type latx_avx_single_vmovsd_run, @function
latx_avx_single_vmovsd_run:
    push rbx
    push r12
    push r13
    push r14
    mov r13, rdx
    test ecx, ecx
    jnz .Lvmovsd_selected_mode
    VMOVSD_RUN_STORES 0
    VMOVSD_RUN_LOADS 0
    VMOVSD_RUN_REGISTERS 0
    VMOVSD_RUN_STORES 15
    VMOVSD_RUN_LOADS 15
    VMOVSD_RUN_REGISTERS 15
    jmp .Lvmovsd_run_done
.Lvmovsd_selected_mode:
    cmp ecx, 1
    jne .Lvmovsd_mode2
    VMOVSD_RUN_STORES 0
    jmp .Lvmovsd_run_done
.Lvmovsd_mode2:
    cmp ecx, 2
    jne .Lvmovsd_mode3
    VMOVSD_RUN_LOADS 0
    jmp .Lvmovsd_run_done
.Lvmovsd_mode3:
    cmp ecx, 3
    jne .Lvmovsd_mode4
    VMOVSD_RUN_REGISTERS 0
    jmp .Lvmovsd_run_done
.Lvmovsd_mode4:
    cmp ecx, 4
    jne .Lvmovsd_mode5
    VMOVSD_RUN_STORES 15
    jmp .Lvmovsd_run_done
.Lvmovsd_mode5:
    cmp ecx, 5
    jne .Lvmovsd_mode6
    VMOVSD_RUN_LOADS 15
    jmp .Lvmovsd_run_done
.Lvmovsd_mode6:
    VMOVSD_RUN_REGISTERS 15
.Lvmovsd_run_done:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vmovsd_observe_marker
    .type latx_avx_single_vmovsd_observe_marker, @function
latx_avx_single_vmovsd_observe_marker:
    vmovsd xmm14, xmm14, xmm14
    vmovsd xmm14, xmm14, xmm14
    ret

    .macro VMOVSD_FAULT_PREPARE restore
    test \restore, \restore
    jz .Lvmovsd_fault_skip_restore_\@
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vmovsd_xrstor_area]
.Lvmovsd_fault_skip_restore_\@:
    movdqu xmm15, xmmword ptr [rip + vmovsd_fault_initial]
    .endm

    .globl latx_avx_single_vmovsd_fault_load
    .type latx_avx_single_vmovsd_fault_load, @function
latx_avx_single_vmovsd_fault_load:
    VMOVSD_FAULT_PREPARE esi
    vmovsd xmm15, qword ptr [rdi]
    ret

    .globl latx_avx_single_vmovsd_fault_store
    .type latx_avx_single_vmovsd_fault_store, @function
latx_avx_single_vmovsd_fault_store:
    VMOVSD_FAULT_PREPARE esi
    vmovsd qword ptr [rdi], xmm15
    ret

    .globl latx_avx_single_vmovsd_fault_observe
    .type latx_avx_single_vmovsd_fault_observe, @function
latx_avx_single_vmovsd_fault_observe:
    jmp latx_avx_single_vmovsd_observe_marker

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .data
    .balign 32
    .globl latx_avx_vmovsd_rip_source
latx_avx_vmovsd_rip_source:
    .quad 0xfedcba9876543210

    .balign 32
    .globl latx_avx_vmovsd_rip_store
latx_avx_vmovsd_rip_store:
    .fill 32, 1, 0xa5

    .balign 32
vmovsd_target_seed:
    .quad 0x0706050403020100, 0x8f8e8d8c8b8a8988
vmovsd_store_source:
    .quad 0x1122334455667788, 0x99aabbccddeeff00
vmovsd_src1:
    .quad 0x0123456789abcdef, 0xfedcba9876543210
vmovsd_src2:
    .quad 0x8000000000000001, 0x7ffffffffffffffe
vmovsd_fault_initial:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850

    .balign 64
vmovsd_xrstor_area:
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
vmovsd_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
