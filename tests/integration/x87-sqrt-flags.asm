; SPDX-License-Identifier: GPL-2.0-only
%ifidn __OUTPUT_FORMAT__, elf64
BITS 64
default rel
%else
BITS 32
%endif

; No SSE or libc: observe FSQRT flags before any result conversion.
%macro sqrt_case 4
    mov ebx, %4
    fninit
    mov word [control], %1
    fldcw [control]
    fld tword [sentinel]
    fld tword [%2]
    fsqrt
    fnstsw ax
    and ax, 0x383f
    cmp ax, 0x3000 | %3  ; FSQRT must also leave TOP unchanged.
    jne exit
    fstp tword [result]
    fstp tword [result]
    cmp dword [result], 0
    jne exit
    cmp dword [result + 4], 0xc0000000
    jne exit
    cmp word [result + 8], 0x4000
    jne exit
%endmacro

section .text
global _start
_start:
%assign mode 0
%assign case_id 1
%rep 4
    ; All three defined x87 precision controls and all rounding modes.
%assign precision 0
%rep 3
%if precision = 0
%assign pc 0
%elif precision = 1
%assign pc 0x200
%else
%assign pc 0x300
%endif
    sqrt_case (0x7f | pc | (mode << 10)), two, 0x20, case_id
%assign case_id case_id + 1
    sqrt_case (0x7f | pc | (mode << 10)), four, 0, case_id
%assign case_id case_id + 1
    sqrt_case (0x7f | pc | (mode << 10)), negative_one, 1, case_id
%assign case_id case_id + 1
%assign precision precision + 1
%endrep
%assign mode mode + 1
%endrep
    ; An exact operation must preserve an earlier sticky precision flag.
    mov ebx, 37
    fninit
    fld tword [two]
    fsqrt
    fstp tword [result]
    fld tword [four]
    fsqrt
    fnstsw ax
    and ax, 0x3f
    cmp ax, 0x20
    jne exit
    xor ebx, ebx
exit:
%ifidn __OUTPUT_FORMAT__, elf64
    mov edi, ebx
    mov eax, 60
    syscall
%else
    mov eax, 1
    int 0x80
%endif

section .data align=16
two: dq 0x8000000000000000
     dw 0x4000
four: dq 0x8000000000000000
      dw 0x4001
negative_one: dq 0x8000000000000000
              dw 0xbfff
sentinel: dq 0xc000000000000000
          dw 0x4000
control: dw 0
section .bss align=16
result: resb 10
section .note.GNU-stack noalloc noexec nowrite progbits
