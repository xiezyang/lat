/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_vmovups_xmm_load
    .type latx_avx_single_vmovups_xmm_load, @function
latx_avx_single_vmovups_xmm_load:
    vmovups xmm0, xmmword ptr [rdi]
    vmovups xmmword ptr [rsi], xmm0
    ret

    .globl latx_avx_single_vmovups_ymm_load
    .type latx_avx_single_vmovups_ymm_load, @function
latx_avx_single_vmovups_ymm_load:
    vmovups ymm15, ymmword ptr [rdi]
    vmovups ymmword ptr [rsi], ymm15
    ret

    .globl latx_avx_single_vmovups_xmm_store
    .type latx_avx_single_vmovups_xmm_store, @function
latx_avx_single_vmovups_xmm_store:
    vmovups xmm0, xmmword ptr [rip + vmovups_pattern_xmm]
    vmovups xmmword ptr [rdi], xmm0
    ret

    .globl latx_avx_single_vmovups_ymm_store
    .type latx_avx_single_vmovups_ymm_store, @function
latx_avx_single_vmovups_ymm_store:
    vmovups ymm15, ymmword ptr [rip + vmovups_pattern_ymm]
    vmovups ymmword ptr [rdi], ymm15
    ret

    .macro VMOVUPS_REGISTER_SETUP
    vmovups ymm0, ymmword ptr [rip + vmovups_register_pattern0]
    vmovups ymm15, ymmword ptr [rip + vmovups_register_pattern15]
    .endm

    .macro VMOVUPS_REGISTER_OBSERVE
    vmovups ymm14, ymm14
    .endm

    .macro VMOVUPS_REGISTER_SAVE reg
    test rdi, rdi
    jz .Lvmovups_register_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vmovups_xsave_area + 160 + (\reg) * 16], \
        xmm14
    movdqu xmmword ptr [rip + vmovups_xsave_area + 576 + (\reg) * 16], \
        xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vmovups_xsave_area]
    movdqu xmm14, \
        xmmword ptr [rip + vmovups_xsave_area + 160 + (\reg) * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, \
        xmmword ptr [rip + vmovups_xsave_area + 576 + (\reg) * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvmovups_register_skip_save_\@:
    .endm

    .globl latx_avx_single_vmovups_registers
    .type latx_avx_single_vmovups_registers, @function
latx_avx_single_vmovups_registers:
    VMOVUPS_REGISTER_SETUP
    vmovups xmm15, xmm0
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 15

    VMOVUPS_REGISTER_SETUP
    vmovups xmm0, xmm15
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 0

    VMOVUPS_REGISTER_SETUP
    vmovups xmm15, xmm15
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 15

    VMOVUPS_REGISTER_SETUP
    vmovups xmm0, xmm0
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 0

    VMOVUPS_REGISTER_SETUP
    vmovups ymm15, ymm0
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 15

    VMOVUPS_REGISTER_SETUP
    vmovups ymm0, ymm15
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 0

    VMOVUPS_REGISTER_SETUP
    vmovups ymm15, ymm15
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 15

    VMOVUPS_REGISTER_SETUP
    vmovups ymm0, ymm0
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 0

    VMOVUPS_REGISTER_SETUP
    vmovups xmm15, xmmword ptr [rsi + 1]
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 15

    VMOVUPS_REGISTER_SETUP
    vmovups ymm0, ymmword ptr [rsi + 31]
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 0

    VMOVUPS_REGISTER_SETUP
    vmovups xmmword ptr [rsi + 1024], xmm15
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 15

    VMOVUPS_REGISTER_SETUP
    vmovups ymmword ptr [rsi + 2048], ymm0
    VMOVUPS_REGISTER_OBSERVE
    VMOVUPS_REGISTER_SAVE 0
    ret

    .globl latx_avx_single_vmovups_fault_xmm_load
    .type latx_avx_single_vmovups_fault_xmm_load, @function
latx_avx_single_vmovups_fault_xmm_load:
    movdqu xmm15, xmmword ptr [rip + vmovups_fault_initial]
    vmovups xmm15, xmmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovups_fault_xmm_store
    .type latx_avx_single_vmovups_fault_xmm_store, @function
latx_avx_single_vmovups_fault_xmm_store:
    movdqu xmm15, xmmword ptr [rip + vmovups_fault_initial]
    vmovups xmmword ptr [rdi], xmm15
    ret

    .globl latx_avx_single_vmovups_fault_ymm_load
    .type latx_avx_single_vmovups_fault_ymm_load, @function
latx_avx_single_vmovups_fault_ymm_load:
    vmovups ymm15, ymmword ptr [rip + vmovups_pattern_ymm]
    vmovups ymm15, ymmword ptr [rdi]
    ret

    .globl latx_avx_single_vmovups_fault_ymm_store
    .type latx_avx_single_vmovups_fault_ymm_store, @function
latx_avx_single_vmovups_fault_ymm_store:
    vmovups ymm15, ymmword ptr [rip + vmovups_pattern_ymm]
    vmovups ymmword ptr [rdi], ymm15
    ret

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vmovups_pattern_ymm:
    .quad 0x0706050403020100, 0x0f0e0d0c0b0a0908
    .quad 0x1716151413121110, 0x1f1e1d1c1b1a1918
vmovups_pattern_xmm:
    .quad 0x8877665544332211, 0x0f1e2d3c4b5a6978
vmovups_fault_initial:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vmovups_register_pattern0:
    .quad 0x0706050403020100, 0x0f0e0d0c0b0a0908
    .quad 0x1716151413121110, 0x1f1e1d1c1b1a1918
vmovups_register_pattern15:
    .quad 0x8877665544332211, 0x0f1e2d3c4b5a6978
    .quad 0x1122334455667788, 0x99aabbccddeeff00

    .bss
    .balign 64
vmovups_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
