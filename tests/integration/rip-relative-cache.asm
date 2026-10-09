; SPDX-License-Identifier: GPL-2.0-only
bits 64
default rel
global _start

; Keep accesses together so subsequent translations can reuse the RIP base.
%macro CHECK_PAIR 1
    mov eax, [%1]
    mov edx, [%1 + 4]
    cmp eax, 0x13579bdf
    jne fail
    cmp edx, 0x2468ace0
    jne fail
%endmacro

section .text
_start:
    CHECK_PAIR positive
    CHECK_PAIR negative
    CHECK_PAIR aligned

    ; The difference from the first address fits si12, but the difference
    ; from its rounded base does not.  This must miss rather than truncate.
    mov eax, [edge_before]
    mov edx, [edge_after]
    cmp eax, 0x13579bdf
    jne fail
    cmp edx, 0x2468ace0
    jne fail

    ; Exercise stores and both vector widths through the same interface.
    mov dword [scratch], 0x13579bdf
    mov dword [scratch + 4], 0x2468ace0
    CHECK_PAIR scratch
    vmovdqu xmm0, [vector]
    vmovdqu xmm1, [vector]
    vmovdqu [scratch + 16], xmm0
    vpxor xmm1, xmm1, [scratch + 16]
    vptest xmm1, xmm1
    jne fail
    vmovdqu ymm0, [vector]
    vmovdqu ymm1, [vector]
    vmovdqu [scratch + 32], ymm0
    vpxor ymm1, ymm1, [scratch + 32]
    vptest ymm1, ymm1
    jne fail

    ; LEA needs a complete address, not a base with an omitted displacement.
    lea rax, [positive]
    mov edx, [rax]
    cmp edx, 0x13579bdf
    jne fail
    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, 60
    syscall

section .rodata align=4096
aligned: dd 0x13579bdf, 0x2468ace0
    times 0x124 - ($ - $$) db 0
positive: dd 0x13579bdf, 0x2468ace0
    times 0x7f0 - ($ - $$) db 0
edge_before: dd 0x13579bdf
    times 0x810 - ($ - $$) db 0
edge_after: dd 0x2468ace0
    times 0xf24 - ($ - $$) db 0
negative: dd 0x13579bdf, 0x2468ace0
align 32
vector: dq 0x0011223344556677, 0x8899aabbccddeeff
        dq 0x0123456789abcdef, 0xfedcba9876543210

section .bss align=4096
    resb 0x124
scratch: resb 64

section .note.GNU-stack noalloc noexec nowrite progbits
