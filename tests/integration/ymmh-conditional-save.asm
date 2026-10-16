; SPDX-License-Identifier: GPL-2.0-only
; Also supports nasm -f elf32 for manual i386 validation.
%ifidn __OUTPUT_FORMAT__, elf32
BITS 32
%define NR_YMM 8
%define STACK_SP esp
%define STACK_SAVED ebx
%define FLAGS_RESULT eax
%define PUSH_FLAGS push dword
%define POP_FLAGS popfd
%else
BITS 64
DEFAULT REL
%define NR_YMM 16
%define STACK_SP rsp
%define STACK_SAVED rbx
%define FLAGS_RESULT rax
%define PUSH_FLAGS push qword
%define POP_FLAGS popfq
%endif
GLOBAL _start

; A nonzero group selects one instruction family for RED/GREEN checks.
%ifndef CASE_GROUP
%define CASE_GROUP 0
%endif

; Flags, pending-zero mask, operation. Compare every bit in every YMM:
; the pending set must have zero upper halves, the others must retain them.
%macro CHECK_STATE 3
    %ifidni %3, FX_ZERO
        fninit                  ; Keep all x87 exceptions masked.
        fldz
    %elifidni %3, FX_ONE
        fninit
        fld1
    %elifidni %3, FX_INF
        fninit
        fld qword [positive_inf]
    %elifidni %3, FX_NEG_INF
        fninit
        fld qword [negative_inf]
    %elifidni %3, INTO
        xor eax, eax            ; OF=0 skips the exception helper.
    %elifidni %3, BOUND
        mov eax, 5              ; In range: skips the exception helper.
    %endif
    %assign reg 0
    %rep NR_YMM
        vmovdqu ymm %+ reg, [ones]
        %assign reg reg + 1
    %endrep
    ; Give the sequence its own entry, independently of TB size.
    jmp %%body
%%body:
    %assign reg 0
    %rep NR_YMM
        %if (%2 & (1 << reg))
            vpor xmm %+ reg, xmm %+ reg, xmm %+ reg
        %endif
        %assign reg reg + 1
    %endrep
    mov STACK_SAVED, STACK_SP
    %ifidni %3, POPF
        PUSH_FLAGS %1
        POP_FLAGS               ; TF=0 skips the helper, not the clears.
    %elifidni %3, RDTSC
        rdtsc
    %elifidni %3, RDTSCP
        rdtscp
    %elifidni %3, INTO
        into
    %elifidni %3, BOUND
        bound eax, [bounds]
    %else
        fxtract
    %endif
    %assign reg 0
    %rep NR_YMM
        vmovdqu [actual + 32 * reg], ymm %+ reg
        %assign reg reg + 1
    %endrep
    %ifidni %3, POPF
        %if NR_YMM = 8
            pushfd
        %else
            pushfq
        %endif
        pop FLAGS_RESULT
        and eax, 0x8d5
        cmp eax, (%1 & 0x8d5)
        jne bad
    %endif
    cmp STACK_SP, STACK_SAVED
    jne bad
    %assign reg 0
    %rep NR_YMM
        %assign lane 0
        %rep 8
            %if lane >= 4 && (%2 & (1 << reg))
                cmp dword [actual + 32 * reg + 4 * lane], 0
            %else
                cmp dword [actual + 32 * reg + 4 * lane], -1
            %endif
            jne bad
            %assign lane lane + 1
        %endrep
        %assign reg reg + 1
    %endrep
%endmacro

SECTION .text
_start:
%if CASE_GROUP = 0 || CASE_GROUP = 1
    CHECK_STATE 0x202, 0, POPF
    CHECK_STATE 0x203, 0x8181, POPF
    CHECK_STATE 0xa46, 0xffff, POPF
    CHECK_STATE 0xa46, 0, POPF
    CHECK_STATE 0x202, 0x8181, POPF
    CHECK_STATE 0x203, 0xffff, POPF
    CHECK_STATE 0x203, 0, POPF
    CHECK_STATE 0xa46, 0x8181, POPF
    CHECK_STATE 0x202, 0xffff, POPF
%endif
%if CASE_GROUP = 0 || CASE_GROUP = 2
    CHECK_STATE 0, 0x8181, RDTSC
    CHECK_STATE 0, 0xffff, RDTSC
%endif
%if CASE_GROUP = 0 || CASE_GROUP = 3
    CHECK_STATE 0, 0x8181, RDTSCP
    CHECK_STATE 0, 0xffff, RDTSCP
%endif
%if CASE_GROUP = 0 || CASE_GROUP = 4
    CHECK_STATE 0, 0x8181, FX_ZERO
    CHECK_STATE 0, 0xffff, FX_INF
    CHECK_STATE 0, 0x8181, FX_NEG_INF
    CHECK_STATE 0, 0xffff, FX_ONE
%endif
%if NR_YMM = 8
%if CASE_GROUP = 0 || CASE_GROUP = 5
    CHECK_STATE 0, 0x81, INTO
%endif
%if CASE_GROUP = 0 || CASE_GROUP = 6
    CHECK_STATE 0, 0x81, BOUND
%endif
%endif
%if NR_YMM = 8
    xor ebx, ebx
    mov eax, 1
    int 0x80
%else
    xor edi, edi
    mov eax, 60
    syscall
%endif
bad:
%if NR_YMM = 8
    mov ebx, 1
    mov eax, 1
    int 0x80
%else
    mov edi, 1
    mov eax, 60
    syscall
%endif

SECTION .rodata
ALIGN 32
ones: times 4 dq -1
positive_inf: dq 0x7ff0000000000000
negative_inf: dq 0xfff0000000000000
bounds: dd 0, 10
SECTION .bss
ALIGN 32
actual: resb NR_YMM * 32
SECTION .note.GNU-stack noalloc noexec nowrite progbits
