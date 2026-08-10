/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_vdivsd_daz_run
    .type latx_avx_single_vdivsd_daz_run, @function
latx_avx_single_vdivsd_daz_run:
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vdivsd_daz_xrstor_area]
    mov dword ptr [rip + vdivsd_daz_mxcsr_control], 0x1fc0
    ldmxcsr dword ptr [rip + vdivsd_daz_mxcsr_control]
    movdqu xmm15, xmmword ptr [rip + vdivsd_daz_target_seed]
    movdqu xmm14, xmmword ptr [rip + vdivsd_daz_min_subnormal]
    movdqu xmm13, xmmword ptr [rip + vdivsd_daz_one]
    vdivsd xmm15, xmm14, xmm13

    stmxcsr dword ptr [rip + vdivsd_daz_mxcsr_after]
    pushfq
    pop r11
    call latx_avx_single_vdivsd_daz_observe_marker
    movdqu xmm12, xmm15
    movdqu xmmword ptr [rdi], xmm12
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vdivsd_daz_xsave_area]
    movdqu xmm12, xmmword ptr [rip + vdivsd_daz_xsave_area + 576 + 15 * 16]
    movdqu xmmword ptr [rdi + 16], xmm12
    mov eax, dword ptr [rip + vdivsd_daz_mxcsr_after]
    mov dword ptr [rdi + 32], eax
    mov dword ptr [rdi + 36], 0
    mov qword ptr [rdi + 40], 0
    mov qword ptr [rdi + 48], 0
    mov qword ptr [rdi + 56], 0
    ret

    .globl latx_avx_single_vdivsd_daz_observe_marker
latx_avx_single_vdivsd_daz_observe_marker:
    ret

    .section .rodata
    .balign 16
vdivsd_daz_target_seed: .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vdivsd_daz_min_subnormal: .quad 1, 0x0123456789abcdef
vdivsd_daz_one: .quad 0x3ff0000000000000, 0xfedcba9876543210

    .section .data
    .balign 64
vdivsd_daz_xrstor_area:
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
vdivsd_daz_xsave_area: .zero 4096
vdivsd_daz_mxcsr_control: .zero 4
vdivsd_daz_mxcsr_after: .zero 4
    .section .note.GNU-stack,"",@progbits
