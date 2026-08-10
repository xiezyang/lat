/* SPDX-License-Identifier: GPL-2.0-only */

    .intel_syntax noprefix
    .text

#if defined(WI1870_VCOMISS)
# define WI1870_COMIS vcomiss
# define WI1870_WIDTH 4
#elif defined(WI1870_VUCOMISS)
# define WI1870_COMIS vucomiss
# define WI1870_WIDTH 4
#else
# define WI1870_COMIS vcomisd
# define WI1870_WIDTH 8
#endif

    .macro WI1870_SET_MXCSR value
    mov dword ptr [rip + wi1870_mxcsr], \value
    ldmxcsr dword ptr [rip + wi1870_mxcsr]
    .endm

    .macro WI1870_SET_FLAGS
    push 0xad7
    popfq
    .endm

    .macro WI1870_SAVE source
    pushfq
    pop rax
    mov qword ptr [rdi], rax
    stmxcsr dword ptr [rdi + 8]
    mov dword ptr [rdi + 12], 0
    movdqu xmm12, \source
    movdqu xmmword ptr [rdi + 16], xmm12
    add rdi, 32
    .endm

    .macro WI1870_REG_CASE left, right
    WI1870_SET_MXCSR 0x1f80
    movdqu \left, xmmword ptr [rip + wi1870_left]
    movdqu \right, xmmword ptr [rip + wi1870_right]
    WI1870_SET_FLAGS
    WI1870_COMIS \left, \right
    WI1870_SAVE \left
    .endm

    .macro WI1870_MEM_CASE offset
    WI1870_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + wi1870_left]
    lea rbx, [rsi + \offset]
    WI1870_SET_FLAGS
#if WI1870_WIDTH == 8
    WI1870_COMIS xmm0, qword ptr [rbx]
#else
    WI1870_COMIS xmm0, dword ptr [rbx]
#endif
    WI1870_SAVE xmm0
    .endm

    .globl latx_avx_wi1870_comis_run
    .type latx_avx_wi1870_comis_run, @function
latx_avx_wi1870_comis_run:
    push rbx
    WI1870_REG_CASE xmm0, xmm1
    WI1870_REG_CASE xmm15, xmm14
    WI1870_SET_MXCSR 0x1fc0
    movdqu xmm15, xmmword ptr [rip + wi1870_subnormal]
    movdqu xmm14, xmmword ptr [rip + wi1870_zero]
    WI1870_SET_FLAGS
    WI1870_COMIS xmm15, xmm14
    WI1870_SAVE xmm15
    WI1870_SET_MXCSR 0x1f80
    movdqu xmm0, xmmword ptr [rip + wi1870_qnan]
    movdqu xmm1, xmmword ptr [rip + wi1870_one]
    WI1870_SET_FLAGS
    WI1870_COMIS xmm0, xmm1
    WI1870_SAVE xmm0
    WI1870_SET_MXCSR 0x1f80
    movdqu xmm15, xmmword ptr [rip + wi1870_snan]
    movdqu xmm14, xmmword ptr [rip + wi1870_one]
    WI1870_SET_FLAGS
    WI1870_COMIS xmm15, xmm14
    WI1870_SAVE xmm15
    WI1870_MEM_CASE 0
    WI1870_MEM_CASE 4090
    WI1870_MEM_CASE 8193
    WI1870_MEM_CASE 12287
    pop rbx
    ret

    .globl latx_avx_wi1870_comis_unmasked
    .type latx_avx_wi1870_comis_unmasked, @function
latx_avx_wi1870_comis_unmasked:
    cmp edi, 1
    je .Lwi1870_invalid
    cmp edi, 2
    je .Lwi1870_denormal
    WI1870_SET_MXCSR 0x1e80
    movdqu xmm15, xmmword ptr [rip + wi1870_qnan]
    movdqu xmm14, xmmword ptr [rip + wi1870_subnormal]
    jmp .Lwi1870_unmasked_ready
.Lwi1870_invalid:
    WI1870_SET_MXCSR 0x1f00
    movdqu xmm15, xmmword ptr [rip + wi1870_snan]
    movdqu xmm14, xmmword ptr [rip + wi1870_one]
    jmp .Lwi1870_unmasked_ready
.Lwi1870_denormal:
    WI1870_SET_MXCSR 0x1e80
    movdqu xmm15, xmmword ptr [rip + wi1870_subnormal]
    movdqu xmm14, xmmword ptr [rip + wi1870_zero]
.Lwi1870_unmasked_ready:
    WI1870_SET_FLAGS
    .globl latx_avx_wi1870_comis_unmasked_site
latx_avx_wi1870_comis_unmasked_site:
    WI1870_COMIS xmm15, xmm14
    ret

    .globl latx_avx_wi1870_rt_sigreturn
latx_avx_wi1870_rt_sigreturn:
    mov eax, 15
    syscall
    hlt

    .section .rodata
    .balign 16
wi1870_left:
#if WI1870_WIDTH == 8
    .quad 0x4000000000000000, 0x13579bdf2468ace0
wi1870_right:
    .quad 0x3ff0000000000000, 0x13579bdf2468ace0
wi1870_one:
    .quad 0x3ff0000000000000, 0x13579bdf2468ace0
wi1870_zero:
    .quad 0, 0x13579bdf2468ace0
wi1870_subnormal:
    .quad 1, 0x13579bdf2468ace0
wi1870_qnan:
    .quad 0x7ff8000000001234, 0x13579bdf2468ace0
wi1870_snan:
    .quad 0x7ff0000000001234, 0x13579bdf2468ace0
#else
    .long 0x40000000, 0x2468ace0, 0, 0
wi1870_right:
    .long 0x3f800000, 0x2468ace0, 0, 0
wi1870_one:
    .long 0x3f800000, 0x2468ace0, 0, 0
wi1870_zero:
    .long 0, 0x2468ace0, 0, 0
wi1870_subnormal:
    .long 1, 0x2468ace0, 0, 0
wi1870_qnan:
    .long 0x7fc01234, 0x2468ace0, 0, 0
wi1870_snan:
    .long 0x7f801234, 0x2468ace0, 0, 0
#endif

    .section .data
    .balign 4
wi1870_mxcsr:
    .long 0x1f80

    .section .note.GNU-stack,"",@progbits
