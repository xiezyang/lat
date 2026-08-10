/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .globl latx_avx_single_vcvtsd2ss_daz_run
    .type latx_avx_single_vcvtsd2ss_daz_run, @function
latx_avx_single_vcvtsd2ss_daz_run:
    mov eax, 6
    xor edx, edx
    xrstor64 [rip + vcvtsd2ss_daz_xrstor_area]
    mov dword ptr [rip + vcvtsd2ss_daz_mxcsr_control], 0x1fc0
    ldmxcsr dword ptr [rip + vcvtsd2ss_daz_mxcsr_control]
    movdqu xmm15, xmmword ptr [rip + vcvtsd2ss_daz_src1_seed]
    movdqu xmm14, xmmword ptr [rip + vcvtsd2ss_daz_min_subnormal]
    vcvtsd2ss xmm15, xmm15, xmm14

    stmxcsr dword ptr [rip + vcvtsd2ss_daz_mxcsr_after]
    pushfq
    pop r11
    call latx_avx_single_vcvtsd2ss_daz_observe_marker
    movdqu xmm12, xmm15
    movdqu xmmword ptr [rdi], xmm12
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vcvtsd2ss_daz_xsave_area]
    movdqu xmm12, xmmword ptr [rip + vcvtsd2ss_daz_xsave_area + 576 + 15 * 16]
    movdqu xmmword ptr [rdi + 16], xmm12
    mov eax, dword ptr [rip + vcvtsd2ss_daz_mxcsr_after]
    mov dword ptr [rdi + 32], eax
    mov dword ptr [rdi + 36], 0
    mov qword ptr [rdi + 40], 0
    mov qword ptr [rdi + 48], 0
    mov qword ptr [rdi + 56], 0
    ret

    .globl latx_avx_single_vcvtsd2ss_daz_observe_marker
latx_avx_single_vcvtsd2ss_daz_observe_marker:
    ret

    .section .rodata
    .balign 16
vcvtsd2ss_daz_src1_seed: .long 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd
vcvtsd2ss_daz_min_subnormal: .quad 1, 0

    .section .data
    .balign 64
vcvtsd2ss_daz_xrstor_area:
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
vcvtsd2ss_daz_xsave_area: .zero 4096
vcvtsd2ss_daz_mxcsr_control: .zero 4
vcvtsd2ss_daz_mxcsr_after: .zero 4
    .section .note.GNU-stack,"",@progbits
