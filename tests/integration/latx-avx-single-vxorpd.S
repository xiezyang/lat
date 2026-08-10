/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

    .macro VXORPD_SETUP index
    vxorpd ymm15, ymm15, ymm15
    vxorpd ymm0, ymm15, ymmword ptr [rip + vxorpd_masks + ((\index % 6) * 32)]
    vxorpd ymm1, ymm15, ymmword ptr [rip + vxorpd_masks + (((\index + 1) % 6) * 32)]
    vxorpd ymm2, ymm15, ymmword ptr [rip + vxorpd_masks + (((\index + 2) % 6) * 32)]
    vxorpd ymm15, ymm15, ymmword ptr [rip + vxorpd_masks + (((\index + 3) % 6) * 32)]
    movdqu xmm0, xmmword ptr [rip + vxorpd_masks + ((\index % 6) * 32)]
    movdqu xmm1, xmmword ptr [rip + vxorpd_masks + (((\index + 1) % 6) * 32)]
    movdqu xmm2, xmmword ptr [rip + vxorpd_masks + (((\index + 2) % 6) * 32)]
    movdqu xmm15, xmmword ptr [rip + vxorpd_masks + (((\index + 3) % 6) * 32)]
    .endm

    .macro VXORPD_FAULT_SETUP
    vxorpd ymm15, ymm15, ymm15
    vxorpd ymm15, ymm15, ymmword ptr [rip + vxorpd_fault_ymm]
    movdqu xmm15, xmmword ptr [rip + vxorpd_seed_low]
    .endm

    .macro VXORPD_SAVE target
    test rdi, rdi
    jz .Lvxorpd_skip_save_\@
    pxor xmm14, xmm14
    movdqu xmmword ptr [rip + vxorpd_xsave_area + 160 + 15 * 16], xmm14
    movdqu xmmword ptr [rip + vxorpd_xsave_area + 576 + 15 * 16], xmm14
    mov eax, 6
    xor edx, edx
    xsave64 [rip + vxorpd_xsave_area]
    .if \target == 0
      movdqu xmm14, xmmword ptr [rip + vxorpd_xsave_area + 160]
      movdqu xmmword ptr [rdi], xmm14
      movdqu xmm14, xmmword ptr [rip + vxorpd_xsave_area + 576]
      movdqu xmmword ptr [rdi + 16], xmm14
      movdqu xmm14, xmmword ptr [rip + vxorpd_xsave_area + 160 + 15 * 16]
      movdqu xmmword ptr [rdi + 32], xmm14
      movdqu xmm14, xmmword ptr [rip + vxorpd_xsave_area + 576 + 15 * 16]
      movdqu xmmword ptr [rdi + 48], xmm14
    .else
      movdqu xmm14, xmmword ptr [rip + vxorpd_xsave_area + 160 + 15 * 16]
      movdqu xmmword ptr [rdi], xmm14
      movdqu xmm14, xmmword ptr [rip + vxorpd_xsave_area + 576 + 15 * 16]
      movdqu xmmword ptr [rdi + 16], xmm14
      movdqu xmm14, xmmword ptr [rip + vxorpd_xsave_area + 160]
      movdqu xmmword ptr [rdi + 32], xmm14
      movdqu xmm14, xmmword ptr [rip + vxorpd_xsave_area + 576]
      movdqu xmmword ptr [rdi + 48], xmm14
    .endif
    add rdi, 64
.Lvxorpd_skip_save_\@:
    .endm

    .macro VXORPD_REG width, n
    VXORPD_SETUP \n
    .if \width == 128
      .if \n == 0
        vxorpd xmm0, xmm1, xmm2
      .elseif \n == 1
        vxorpd xmm0, xmm0, xmm2
      .elseif \n == 2
        vxorpd xmm0, xmm1, xmm0
      .elseif \n == 3
        vxorpd xmm0, xmm1, xmm1
      .elseif \n == 4
        vxorpd xmm0, xmm0, xmm0
      .elseif \n == 5
        vxorpd xmm15, xmm1, xmm2
      .elseif \n == 6
        vxorpd xmm15, xmm15, xmm2
      .elseif \n == 7
        vxorpd xmm15, xmm1, xmm15
      .elseif \n == 8
        vxorpd xmm15, xmm1, xmm1
      .else
        vxorpd xmm15, xmm15, xmm15
      .endif
    .else
      .if \n == 0
        vxorpd ymm0, ymm1, ymm2
      .elseif \n == 1
        vxorpd ymm0, ymm0, ymm2
      .elseif \n == 2
        vxorpd ymm0, ymm1, ymm0
      .elseif \n == 3
        vxorpd ymm0, ymm1, ymm1
      .elseif \n == 4
        vxorpd ymm0, ymm0, ymm0
      .elseif \n == 5
        vxorpd ymm15, ymm1, ymm2
      .elseif \n == 6
        vxorpd ymm15, ymm15, ymm2
      .elseif \n == 7
        vxorpd ymm15, ymm1, ymm15
      .elseif \n == 8
        vxorpd ymm15, ymm1, ymm1
      .else
        vxorpd ymm15, ymm15, ymm15
      .endif
    .endif
    .if \n < 5
      call latx_avx_single_vxorpd_observe_ymm0
    .else
      call latx_avx_single_vxorpd_observe_ymm15
    .endif
    .if \n < 5
      VXORPD_SAVE 0
    .else
      VXORPD_SAVE 15
    .endif
    .endm

    .macro VXORPD_MEM width, n
    VXORPD_SETUP \n
    .if (\n % 8) == 0
      lea rbx, [rsi]
    .elseif (\n % 8) == 1
      lea rbx, [rsi + 8192]
      mov r12, 3
    .elseif (\n % 8) == 2
      lea rbx, [rsi + 16384]
    .elseif (\n % 8) == 3
      lea rbx, [rsi + 32768]
    .elseif (\n % 8) == 4
      lea rbx, [rsi + 40960]
    .elseif (\n % 8) == 5
      lea rbx, [rsi + 49152]
    .elseif (\n % 8) == 6
      lea rbx, [rsi + 49152]
    .endif

    .if \width == 128
      .if (\n % 8) == 0
        vxorpd xmm0, xmm1, xmmword ptr [rbx]
      .elseif (\n % 8) == 1
        vxorpd xmm0, xmm0, xmmword ptr [rbx + r12 * 2 + 7]
      .elseif (\n % 8) == 2
        vxorpd xmm15, xmm1, xmmword ptr [rbx + 120]
      .elseif (\n % 8) == 3
        vxorpd xmm15, xmm15, xmmword ptr [rsi + 25600]
      .elseif (\n % 8) == 4
        vxorpd xmm0, xmm1, xmmword ptr [rbx + 1]
      .elseif (\n % 8) == 5
        vxorpd xmm15, xmm15, xmmword ptr [rbx + 4080]
      .elseif (\n % 8) == 6
        vxorpd xmm0, xmm0, xmmword ptr [rbx + 4090]
      .else
        vxorpd xmm15, xmm1, xmmword ptr [rip + vxorpd_rip_source]
      .endif
    .else
      .if (\n % 8) == 0
        vxorpd ymm0, ymm1, ymmword ptr [rbx]
      .elseif (\n % 8) == 1
        vxorpd ymm0, ymm0, ymmword ptr [rbx + r12 * 2 + 7]
      .elseif (\n % 8) == 2
        vxorpd ymm15, ymm1, ymmword ptr [rbx + 120]
      .elseif (\n % 8) == 3
        vxorpd ymm15, ymm15, ymmword ptr [rsi + 25600]
      .elseif (\n % 8) == 4
        vxorpd ymm0, ymm1, ymmword ptr [rbx + 1]
      .elseif (\n % 8) == 5
        vxorpd ymm15, ymm15, ymmword ptr [rbx + 4064]
      .elseif (\n % 8) == 6
        vxorpd ymm0, ymm0, ymmword ptr [rbx + 4080]
      .else
        vxorpd ymm15, ymm1, ymmword ptr [rip + vxorpd_rip_source]
      .endif
    .endif
    .if (\n % 4) < 2
      call latx_avx_single_vxorpd_observe_ymm0
    .else
      call latx_avx_single_vxorpd_observe_ymm15
    .endif
    .if (\n % 4) < 2
      VXORPD_SAVE 0
    .else
      VXORPD_SAVE 15
    .endif
    .endm

    .globl latx_avx_single_vxorpd_run
latx_avx_single_vxorpd_run:
    push rbx
    push r12
    .set vxorpd_n, 0
    .rept 10
      VXORPD_REG 128, vxorpd_n
      .set vxorpd_n, vxorpd_n + 1
    .endr
    .set vxorpd_n, 0
    .rept 10
      VXORPD_REG 256, vxorpd_n
      .set vxorpd_n, vxorpd_n + 1
    .endr
    .set vxorpd_n, 0
    .rept 32
      VXORPD_MEM 128, vxorpd_n
      .set vxorpd_n, vxorpd_n + 1
    .endr
    .set vxorpd_n, 0
    .rept 32
      VXORPD_MEM 256, vxorpd_n
      .set vxorpd_n, vxorpd_n + 1
    .endr
    pop r12
    pop rbx
    ret

    .globl latx_avx_single_vxorpd_observe_ymm0
latx_avx_single_vxorpd_observe_ymm0:
    jmp .Lvxorpd_observe
    .globl latx_avx_single_vxorpd_observe_ymm15
latx_avx_single_vxorpd_observe_ymm15:
    jmp .Lvxorpd_observe
.Lvxorpd_observe:
    .globl latx_avx_single_vxorpd_observe_marker
latx_avx_single_vxorpd_observe_marker:
    vxorpd ymm14, ymm14, ymmword ptr [rip + vxorpd_zeros]
    vxorpd ymm14, ymm14, ymmword ptr [rip + vxorpd_zeros]
    ret

    .globl latx_avx_single_vxorpd_flags_preserved
latx_avx_single_vxorpd_flags_preserved:
    sub rsp, 16
    pushfq
    pop rax
    stmxcsr [rsp]
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    vxorpd xmm0, xmm1, xmm2
    pushfq
    pop rdx
    stmxcsr [rsp + 4]
    cmp rax, rdx
    jne .Lvxorpd_flags_fail
    mov ecx, dword ptr [rsp]
    cmp ecx, dword ptr [rsp + 4]
    jne .Lvxorpd_flags_fail
    pushfq
    pop rax
    stmxcsr [rsp]
    vxorpd xmm0, xmm1, xmmword ptr [rip + vxorpd_zeros]
    pushfq
    pop rdx
    stmxcsr [rsp + 4]
    cmp rax, rdx
    jne .Lvxorpd_flags_fail
    mov ecx, dword ptr [rsp]
    cmp ecx, dword ptr [rsp + 4]
    jne .Lvxorpd_flags_fail
    pushfq
    pop rax
    stmxcsr [rsp]
    vxorpd ymm0, ymm1, ymmword ptr [rip + vxorpd_zeros]
    pushfq
    pop rdx
    stmxcsr [rsp + 4]
    cmp rax, rdx
    jne .Lvxorpd_flags_fail
    mov ecx, dword ptr [rsp]
    cmp ecx, dword ptr [rsp + 4]
    jne .Lvxorpd_flags_fail
    pushfq
    pop rax
    stmxcsr [rsp]
    vxorpd ymm0, ymm1, ymm2
    pushfq
    pop rdx
    stmxcsr [rsp + 4]
    cmp rax, rdx
    jne .Lvxorpd_flags_fail
    mov ecx, dword ptr [rsp]
    cmp ecx, dword ptr [rsp + 4]
    jne .Lvxorpd_flags_fail
    mov eax, 1
    add rsp, 16
    ret
.Lvxorpd_flags_fail:
    xor eax, eax
    add rsp, 16
    ret

    .globl latx_avx_single_vxorpd_fault_xmm
latx_avx_single_vxorpd_fault_xmm:
    VXORPD_FAULT_SETUP
    vxorpd xmm15, xmm15, xmmword ptr [rdi]
    ret
    .globl latx_avx_single_vxorpd_fault_ymm
latx_avx_single_vxorpd_fault_ymm:
    VXORPD_FAULT_SETUP
    vxorpd ymm15, ymm15, ymmword ptr [rdi]
    ret
    .globl latx_avx_single_vxorpd_fault_observe
latx_avx_single_vxorpd_fault_observe:
    jmp latx_avx_single_vxorpd_observe_ymm15
    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 32
vxorpd_seed_low: .quad 0x6f13b88ac09d5e27, 0xe4a2097c31d6f850
vxorpd_fault_ymm:
    .quad 0xd739a2f14c806b5e, 0x07e6d5c4b3a29180
    .globl latx_avx_single_vxorpd_fault_ymmh15
latx_avx_single_vxorpd_fault_ymmh15:
    .quad 0x89abcdef01234567, 0xfedcba9876543210
vxorpd_zeros: .quad 0, 0, 0, 0
vxorpd_masks:
    .quad 0, 0, 0, 0
    .quad -1, -1, -1, -1
    .quad 0x8000000000000000, 0, 0x8000000000000000, 0
    .byte 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    .byte 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
    .quad 0x55aa55aa55aa55aa, 0xaa55aa55aa55aa55, 0x55aa55aa55aa55aa, 0xaa55aa55aa55aa55
    .quad 0xd739a2f14c806b5e, 0x07e6d5c4b3a29180, 0x89abcdef01234567, 0xfedcba9876543210
vxorpd_rip_source: .quad 0, -1, 0x8000000000000000, 0x55aa55aa55aa55aa

    .section .bss
    .balign 64
vxorpd_xsave_area: .zero 4096
    .section .note.GNU-stack,"",@progbits
