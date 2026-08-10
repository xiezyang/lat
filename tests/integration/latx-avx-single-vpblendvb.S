/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VPBLENDVB_RESTORE_SEED target
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vpblendvb_xrstor_area]
    .if \target == 0
    movdqu xmm0, xmmword ptr [rip + vpblendvb_seed_low]
    vpblendvb ymm13, ymm0, ymm0, ymm0
    .else
    movdqu xmm15, xmmword ptr [rip + vpblendvb_seed_low]
    vpblendvb ymm13, ymm15, ymm15, ymm15
    .endif
    .endm

    .macro VPBLENDVB_SET_REG reg, pattern
    vpblendvb ymm\reg, ymm13, ymmword ptr [rip + vpblendvb_patterns + ((\pattern % 8) * 32)], ymm13
    .endm

    .macro VPBLENDVB_SET_MASK mask, pattern
    .if \mask == 0
    VPBLENDVB_SET_REG 0, \pattern
    .elseif \mask == 1
    VPBLENDVB_SET_REG 1, \pattern
    .elseif \mask == 2
    VPBLENDVB_SET_REG 2, \pattern
    .elseif \mask == 3
    VPBLENDVB_SET_REG 3, \pattern
    .elseif \mask == 4
    VPBLENDVB_SET_REG 4, \pattern
    .elseif \mask == 5
    VPBLENDVB_SET_REG 5, \pattern
    .elseif \mask == 6
    VPBLENDVB_SET_REG 6, \pattern
    .elseif \mask == 7
    VPBLENDVB_SET_REG 7, \pattern
    .elseif \mask == 8
    VPBLENDVB_SET_REG 8, \pattern
    .elseif \mask == 9
    VPBLENDVB_SET_REG 9, \pattern
    .elseif \mask == 10
    VPBLENDVB_SET_REG 10, \pattern
    .elseif \mask == 11
    VPBLENDVB_SET_REG 11, \pattern
    .elseif \mask == 12
    VPBLENDVB_SET_REG 12, \pattern
    .elseif \mask == 13
    vpblendvb ymm12, ymm13, ymm13, ymm13
    vpblendvb ymm13, ymm12, ymmword ptr [rip + vpblendvb_patterns + ((\pattern % 8) * 32)], ymm12
    .elseif \mask == 14
    VPBLENDVB_SET_REG 14, \pattern
    .else
    VPBLENDVB_SET_REG 15, \pattern
    .endif
    .endm

    .macro VPBLENDVB_SETUP target, mask, index
    VPBLENDVB_SET_REG 0, (\index + 0)
    VPBLENDVB_SET_REG 1, (\index + 1)
    VPBLENDVB_SET_REG 2, (\index + 2)
    VPBLENDVB_SET_REG 3, (\index + 3)
    VPBLENDVB_SET_REG 4, (\index + 4)
    VPBLENDVB_SET_REG 5, (\index + 5)
    VPBLENDVB_SET_REG 6, (\index + 6)
    VPBLENDVB_SET_REG 15, (\index + 7)
    VPBLENDVB_SET_MASK \mask, (\index + 5)
    .endm

    .macro VPBLENDVB_RESTORE_MASTER mask
    .if \mask == 13
    vpblendvb ymm13, ymm12, ymm12, ymm12
    .endif
    .endm

    .macro VPBLENDVB_SAVE_RESULT target
    test rdi, rdi
    jz .Lvpblendvb_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vpblendvb_xsave_area + 160 + \target * 16], xmm14
    movdqu xmmword ptr [rip + vpblendvb_xsave_area + 576 + \target * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vpblendvb_xsave_area]
    movdqu xmm14, xmmword ptr [rip + vpblendvb_xsave_area + 160 + \target * 16]
    movdqu xmmword ptr [rdi], xmm14
    movdqu xmm14, xmmword ptr [rip + vpblendvb_xsave_area + 576 + \target * 16]
    movdqu xmmword ptr [rdi + 16], xmm14
    add rdi, 32
.Lvpblendvb_skip_save_\@:
    .endm

    .macro VPBLENDVB_RECORD_RESULT target
    call latx_avx_single_vpblendvb_observe_marker
    VPBLENDVB_SAVE_RESULT \target
    .endm

    .macro VPBLENDVB_MASK_CASE width, target, mask
    VPBLENDVB_SETUP \target, \mask, (\mask + \width + \target)
    .if \mask == 0
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm0
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm0
        .endif
    .elseif \mask == 1
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm1
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm1
        .endif
    .elseif \mask == 2
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm2
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm2
        .endif
    .elseif \mask == 3
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm3
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm3
        .endif
    .elseif \mask == 4
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm4
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm4
        .endif
    .elseif \mask == 5
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm5
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm5
        .endif
    .elseif \mask == 6
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm6
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm6
        .endif
    .elseif \mask == 7
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm7
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm7
        .endif
    .elseif \mask == 8
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm8
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm8
        .endif
    .elseif \mask == 9
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm9
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm9
        .endif
    .elseif \mask == 10
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm10
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm10
        .endif
    .elseif \mask == 11
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm11
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm11
        .endif
    .elseif \mask == 12
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm12
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm12
        .endif
    .elseif \mask == 13
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm13
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm13
        .endif
    .elseif \mask == 14
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm14
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm14
        .endif
    .else
        .if \width == 128
        vpblendvb xmm\target, xmm6, xmm1, xmm15
        .else
        vpblendvb ymm\target, ymm6, ymm1, ymm15
        .endif
    .endif
    VPBLENDVB_RECORD_RESULT \target
    VPBLENDVB_RESTORE_MASTER \mask
    .endm

    .macro VPBLENDVB_ALIAS_CASE width, target, variant
    VPBLENDVB_SETUP \target, 3, (\variant + \width + \target)
    .if \width == 128
        .if \variant == 0
        vpblendvb xmm\target, xmm\target, xmm2, xmm3
        .elseif \variant == 1
        vpblendvb xmm\target, xmm2, xmm\target, xmm3
        .elseif \variant == 2
        vpblendvb xmm\target, xmm2, xmm3, xmm\target
        .elseif \variant == 3
        vpblendvb xmm\target, xmm2, xmm2, xmm3
        .elseif \variant == 4
        vpblendvb xmm\target, xmm3, xmm2, xmm3
        .elseif \variant == 5
        vpblendvb xmm\target, xmm2, xmm3, xmm3
        .else
        vpblendvb xmm\target, xmm\target, xmm\target, xmm\target
        .endif
    .else
        .if \variant == 0
        vpblendvb ymm\target, ymm\target, ymm2, ymm3
        .elseif \variant == 1
        vpblendvb ymm\target, ymm2, ymm\target, ymm3
        .elseif \variant == 2
        vpblendvb ymm\target, ymm2, ymm3, ymm\target
        .elseif \variant == 3
        vpblendvb ymm\target, ymm2, ymm2, ymm3
        .elseif \variant == 4
        vpblendvb ymm\target, ymm3, ymm2, ymm3
        .elseif \variant == 5
        vpblendvb ymm\target, ymm2, ymm3, ymm3
        .else
        vpblendvb ymm\target, ymm\target, ymm\target, ymm\target
        .endif
    .endif
    VPBLENDVB_RECORD_RESULT \target
    .endm

    .macro VPBLENDVB_PREPARE_ADDRESS address_variant
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

    .macro VPBLENDVB_XMM_MEM dest, src, mask, address_variant
    .if \address_variant == 0
    vpblendvb \dest, \src, xmmword ptr [rbx], \mask
    .elseif \address_variant == 1
    vpblendvb \dest, \src, xmmword ptr [rbx + r12 * 2 + 7], \mask
    .elseif \address_variant == 2
    vpblendvb \dest, \src, xmmword ptr [rbx + 120], \mask
    .elseif \address_variant == 3
    vpblendvb \dest, \src, xmmword ptr [rsi + 25600], \mask
    .elseif \address_variant == 4
    vpblendvb \dest, \src, xmmword ptr [rbx + 1], \mask
    .elseif \address_variant == 5
    vpblendvb \dest, \src, xmmword ptr [rbx + 4080], \mask
    .elseif \address_variant == 6
    vpblendvb \dest, \src, xmmword ptr [rbx + 4090], \mask
    .else
    vpblendvb \dest, \src, xmmword ptr [rip + vpblendvb_rip_source], \mask
    .endif
    .endm

    .macro VPBLENDVB_YMM_MEM dest, src, mask, address_variant
    .if \address_variant == 0
    vpblendvb \dest, \src, ymmword ptr [rbx], \mask
    .elseif \address_variant == 1
    vpblendvb \dest, \src, ymmword ptr [rbx + r12 * 2 + 7], \mask
    .elseif \address_variant == 2
    vpblendvb \dest, \src, ymmword ptr [rbx + 120], \mask
    .elseif \address_variant == 3
    vpblendvb \dest, \src, ymmword ptr [rsi + 25600], \mask
    .elseif \address_variant == 4
    vpblendvb \dest, \src, ymmword ptr [rbx + 1], \mask
    .elseif \address_variant == 5
    vpblendvb \dest, \src, ymmword ptr [rbx + 4064], \mask
    .elseif \address_variant == 6
    vpblendvb \dest, \src, ymmword ptr [rbx + 4080], \mask
    .else
    vpblendvb \dest, \src, ymmword ptr [rip + vpblendvb_rip_source], \mask
    .endif
    .endm

    .macro VPBLENDVB_MEM_CASE width, target, index
    .if (\index % 3) == 0
    VPBLENDVB_SETUP \target, 3, (\index + \width + \target)
    .elseif (\index % 3) == 1
    VPBLENDVB_SETUP \target, \target, (\index + \width + \target)
    .else
    VPBLENDVB_SETUP \target, 3, (\index + \width + \target)
    .endif
    VPBLENDVB_PREPARE_ADDRESS (\index % 8)
    .if \width == 128
        .if (\index % 3) == 0
        VPBLENDVB_XMM_MEM xmm\target, xmm6, xmm3, (\index % 8)
        .elseif (\index % 3) == 1
        VPBLENDVB_XMM_MEM xmm\target, xmm6, xmm\target, (\index % 8)
        .else
        VPBLENDVB_XMM_MEM xmm\target, xmm\target, xmm3, (\index % 8)
        .endif
    .else
        .if (\index % 3) == 0
        VPBLENDVB_YMM_MEM ymm\target, ymm6, ymm3, (\index % 8)
        .elseif (\index % 3) == 1
        VPBLENDVB_YMM_MEM ymm\target, ymm6, ymm\target, (\index % 8)
        .else
        VPBLENDVB_YMM_MEM ymm\target, ymm\target, ymm3, (\index % 8)
        .endif
    .endif
    VPBLENDVB_RECORD_RESULT \target
    .endm

    .macro VPBLENDVB_RAW_CASE width, low_nibble
    VPBLENDVB_SETUP 0, 0, (\low_nibble + \width)
    .if \width == 128
    .byte 0xc4, 0xe3, 0x49, 0x4c, 0xc1, \low_nibble
    .else
    .byte 0xc4, 0xe3, 0x4d, 0x4c, 0xc1, \low_nibble
    .endif
    VPBLENDVB_RECORD_RESULT 0
    .endm

    .macro VPBLENDVB_RUN_TARGET target
    .set vpblendvb_mask, 0
    .rept 16
        VPBLENDVB_MASK_CASE 128, \target, vpblendvb_mask
        .set vpblendvb_mask, vpblendvb_mask + 1
    .endr
    .set vpblendvb_mask, 0
    .rept 16
        VPBLENDVB_MASK_CASE 256, \target, vpblendvb_mask
        .set vpblendvb_mask, vpblendvb_mask + 1
    .endr

    .set vpblendvb_alias, 0
    .rept 7
        VPBLENDVB_ALIAS_CASE 128, \target, vpblendvb_alias
        .set vpblendvb_alias, vpblendvb_alias + 1
    .endr
    .set vpblendvb_alias, 0
    .rept 7
        VPBLENDVB_ALIAS_CASE 256, \target, vpblendvb_alias
        .set vpblendvb_alias, vpblendvb_alias + 1
    .endr

    .set vpblendvb_mem, 0
    .rept 8
        VPBLENDVB_MEM_CASE 128, \target, vpblendvb_mem
        .set vpblendvb_mem, vpblendvb_mem + 1
    .endr
    .set vpblendvb_mem, 0
    .rept 8
        VPBLENDVB_MEM_CASE 256, \target, vpblendvb_mem
        .set vpblendvb_mem, vpblendvb_mem + 1
    .endr
    .endm

    .globl latx_avx_single_vpblendvb_run
    .type latx_avx_single_vpblendvb_run, @function
latx_avx_single_vpblendvb_run:
    push rbx
    push r12
    push r13
    mov r13d, edx
    cmp r13d, 2
    je .Lvpblendvb_run_target15
    VPBLENDVB_RESTORE_SEED 0
    VPBLENDVB_RUN_TARGET 0
    .set vpblendvb_raw, 0
    .rept 16
        VPBLENDVB_RAW_CASE 128, vpblendvb_raw
        .set vpblendvb_raw, vpblendvb_raw + 1
    .endr
    .set vpblendvb_raw, 0
    .rept 16
        VPBLENDVB_RAW_CASE 256, vpblendvb_raw
        .set vpblendvb_raw, vpblendvb_raw + 1
    .endr
    cmp r13d, 1
    je .Lvpblendvb_run_done
.Lvpblendvb_run_target15:
    VPBLENDVB_RESTORE_SEED 15
    VPBLENDVB_RUN_TARGET 15
.Lvpblendvb_run_done:
    pop r13
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vpblendvb_observe_marker
    .type latx_avx_single_vpblendvb_observe_marker, @function
latx_avx_single_vpblendvb_observe_marker:
    vpblendvb ymm14, ymm14, ymm14, ymm14
    vpblendvb ymm14, ymm14, ymm14, ymm14
    ret

    .macro VPBLENDVB_FAULT_PREPARE
    VPBLENDVB_RESTORE_SEED 15
    VPBLENDVB_SETUP 15, 15, 7
    .endm

    .globl latx_avx_single_vpblendvb_fault_xmm
    .type latx_avx_single_vpblendvb_fault_xmm, @function
latx_avx_single_vpblendvb_fault_xmm:
    VPBLENDVB_FAULT_PREPARE
    vpblendvb xmm15, xmm6, xmmword ptr [rdi], xmm15
    ret

    .globl latx_avx_single_vpblendvb_fault_ymm
    .type latx_avx_single_vpblendvb_fault_ymm, @function
latx_avx_single_vpblendvb_fault_ymm:
    VPBLENDVB_FAULT_PREPARE
    vpblendvb ymm15, ymm6, ymmword ptr [rdi], ymm15
    ret

    .globl latx_avx_single_vpblendvb_fault_observe
    .type latx_avx_single_vpblendvb_fault_observe, @function
latx_avx_single_vpblendvb_fault_observe:
    jmp latx_avx_single_vpblendvb_observe_marker

    .globl latx_avx_single_rt_sigreturn
    .type latx_avx_single_rt_sigreturn, @function
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vpblendvb_seed_low:
    .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vpblendvb_patterns:
    .byte 0x00, 0x01, 0x7f, 0x80, 0x81, 0xff, 0x55, 0xaa
    .byte 0x10, 0x90, 0x20, 0xa0, 0x30, 0xb0, 0x40, 0xc0
    .byte 0x00, 0x80, 0x7f, 0xff, 0x01, 0x81, 0x55, 0xd5
    .byte 0x2a, 0xaa, 0x3b, 0xbb, 0x4c, 0xcc, 0x5d, 0xdd
    .rept 32
    .byte 0x00
    .endr
    .rept 32
    .byte 0xff
    .endr
    .byte 0x7f, 0x80, 0x7f, 0x80, 0x7f, 0x80, 0x7f, 0x80
    .byte 0x01, 0x81, 0x02, 0x82, 0x03, 0x83, 0x04, 0x84
    .byte 0x05, 0x85, 0x06, 0x86, 0x07, 0x87, 0x08, 0x88
    .byte 0x09, 0x89, 0x0a, 0x8a, 0x0b, 0x8b, 0x0c, 0x8c
    .byte 0x00, 0xff, 0x01, 0xfe, 0x02, 0xfd, 0x03, 0xfc
    .byte 0x04, 0xfb, 0x05, 0xfa, 0x06, 0xf9, 0x07, 0xf8
    .byte 0x08, 0xf7, 0x09, 0xf6, 0x0a, 0xf5, 0x0b, 0xf4
    .byte 0x0c, 0xf3, 0x0d, 0xf2, 0x0e, 0xf1, 0x0f, 0xf0
    .byte 0x7e, 0x7f, 0x80, 0x81, 0xfe, 0xff, 0x00, 0x01
    .byte 0x11, 0x91, 0x22, 0xa2, 0x33, 0xb3, 0x44, 0xc4
    .byte 0x55, 0xd5, 0x66, 0xe6, 0x77, 0xf7, 0x08, 0x88
    .byte 0x19, 0x99, 0x2a, 0xaa, 0x3b, 0xbb, 0x4c, 0xcc
    .byte 0x80, 0x00, 0x81, 0x01, 0x82, 0x02, 0x83, 0x03
    .byte 0x84, 0x04, 0x85, 0x05, 0x86, 0x06, 0x87, 0x07
    .byte 0x88, 0x08, 0x89, 0x09, 0x8a, 0x0a, 0x8b, 0x0b
    .byte 0x8c, 0x0c, 0x8d, 0x0d, 0x8e, 0x0e, 0x8f, 0x0f
vpblendvb_rip_source:
    .byte 0x00, 0xff, 0x7f, 0x80, 0x01, 0x81, 0x55, 0xaa
    .byte 0x10, 0x90, 0x20, 0xa0, 0x30, 0xb0, 0x40, 0xc0
    .byte 0x11, 0x91, 0x22, 0xa2, 0x33, 0xb3, 0x44, 0xc4
    .byte 0x7e, 0xfe, 0x7f, 0xff, 0x00, 0x80, 0x01, 0x81

    .section .data
    .balign 64
vpblendvb_xrstor_area:
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
vpblendvb_xsave_area:
    .zero 4096

    .section .note.GNU-stack,"",@progbits
