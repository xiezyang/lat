; SPDX-License-Identifier: GPL-2.0-only
; Raw-bit MOVSS/MOVSD checks: memory/register, legacy/VEX, both guest ABIs.
%if __BITS__ = 64
default rel
%endif
global _start
%macro CHECK 1
    vmovdqu [actual], ymm%1
%assign k 0
%rep 8
    mov eax, [actual + 4*k]
    cmp eax, [expected + 4*k]
    jne fail
%assign k k+1
%endrep
%endmacro
%macro EXPECT 2
%assign k 0
%rep 8
    mov dword [expected + 4*k], 0
%assign k k+1
%endrep
    mov eax, [esi]
    mov [expected], eax
%if %1 = 64
    mov eax, [esi+4]
    mov [expected+4], eax
%endif
%if %2 = 1
%assign k 4
%rep 4
    mov eax, [initial+4*k]
    mov [expected+4*k], eax
%assign k k+1
%endrep
%endif
%endmacro
%macro CASES 1
    ; Legacy memory forms zero the rest of XMM and preserve YMM[255:128].
    EXPECT 32, 1
    vmovdqu ymm%1, [initial]
    movss xmm%1, [esi]
    CHECK %1
    EXPECT 64, 1
    vmovdqu ymm%1, [initial]
    movsd xmm%1, [esi]
    CHECK %1
    ; Legacy register forms preserve the rest of the destination.
    vmovdqu ymm%1, [initial]
    vmovdqu ymm2, [source]
    movss xmm%1, xmm2
    EXPECT 32, 1
%assign k 0
%rep 4
    mov eax, [initial+4*k]
    mov [expected+4*k], eax
%assign k k+1
%endrep
    mov eax, [source]
    mov [expected], eax
    CHECK %1
    vmovdqu ymm%1, [initial]
    movsd xmm%1, xmm2
    mov eax, [source+4]
    mov [expected+4], eax
    CHECK %1
    ; VEX memory forms zero every bit above the loaded value.
    EXPECT 32, 0
    vmovdqu ymm%1, [initial]
    vmovss xmm%1, [esi]
    CHECK %1
    EXPECT 64, 0
    vmovdqu ymm%1, [initial]
    vmovsd xmm%1, [esi]
    CHECK %1
%endmacro
section .text
_start:
    mov ecx, 16
    mov esi, values
next_value:
    CASES 0
    CASES 3
    CASES 7
%if __BITS__ = 64
    CASES 8
    CASES 15
%endif
    add esi, 8
    dec ecx
    jnz next_value
    ; A valid scalar at the very end of a readable page must not over-read.
%if __BITS__ = 64
    mov eax, 10
    mov edi, guard_area+4096
    mov esi, 4096
    xor edx, edx
    syscall
%else
    mov eax, 125
    mov ebx, guard_area+4096
    mov ecx, 4096
    xor edx, edx
    int 0x80
%endif
    test eax, eax
    jnz fail
    mov esi, guard_area+4092
    mov dword [esi], 0x7f801234
    EXPECT 32, 1
    vmovdqu ymm0, [initial]
    movss xmm0, [esi]
    CHECK 0
    mov esi, guard_area+4088
    mov dword [esi], 0x12345678
    mov dword [esi+4], 0x7ff00000
    EXPECT 64, 1
    vmovdqu ymm0, [initial]
    movsd xmm0, [esi]
    CHECK 0
    xor ebx, ebx
    jmp exit
fail:
    mov ebx, 1
exit:
%if __BITS__ = 64
    mov edi, ebx
    mov eax, 60
    syscall
%else
    mov eax, 1
    int 0x80
%endif
section .rodata align=32
initial: dd 0x44444444, 0x33333333, 0x22222222, 0x11111111
         dd 0x12345678, 0x76543210, 0x55aa55aa, 0xa55aa55a
source:  dd 0x7f801234, 0x7ff00000, 0x10203040, 0x50607080
         dd 9, 10, 11, 12
values: dq 0, -1, 0x8000000080000000, 0x3ff000003f800000
        dq 0xbff00000bf800000, 0x7ff000007f800000
        dq 0x7ff800007fc01234, 0x7ff000007f801234
        dq 1, 0x000fffffffffffff, 0x0010000000800000
        dq 0x7fefffff7f7fffff, 0xaaaaaaaa55555555, 0x55555555aaaaaaaa
        dq 0x0123456789abcdef, 0x8000000000000001
section .bss align=4096
guard_area: resb 8192
actual: resb 32
expected: resb 32
section .note.GNU-stack noalloc noexec nowrite progbits
