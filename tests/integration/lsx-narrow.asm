; SPDX-License-Identifier: GPL-2.0-only
; Hash all 256 result bits, including YMM upper halves, for fixed AVX cases.
; -DRECORD=1 writes native-x86 reference hashes instead of checking them.
%if __BITS__ = 64
default rel
%define ADDR rcx
%else
%define ADDR ecx
%endif
global _start
%ifndef CASE_GROUP
%define CASE_GROUP 0
%endif

%macro RESET 2
    vmovdqu ymm0, [%1]
    vmovdqu ymm1, [%1]
    vmovdqu ymm2, [%2]
    vmovdqu ymm3, [ones]
%endmacro

%macro HASH 1
    vmovdqu [actual], ymm %+ %1
    mov eax, [digest]
%assign word 0
%rep 8
    xor eax, [actual + word * 4]
    imul eax, eax, 16777619
%assign word word + 1
%endrep
    mov [digest], eax
%endmacro

; Distinct destination, either source alias, equal sources, memory source.
%macro BIN 4
%assign dest 0
%rep 3
    RESET %3, %4
    %1 %2 %+ dest, %2 %+ 1, %2 %+ 2
    HASH dest
%assign dest dest + 1
%endrep
    RESET %3, %4
    %1 %2 %+ 0, %2 %+ 1, %2 %+ 1
    HASH 0
%ifidni %1, vmovss
%elifidni %1, vmovsd
%else
    RESET %3, %4
    %1 %2 %+ 0, %2 %+ 1, [%4]
    HASH 0
%endif
%endmacro

%macro UNARY 4
    RESET %3, %4
    %1 %2 %+ 0, %2 %+ 1
    HASH 0
    RESET %3, %4
    %1 %2 %+ 1, %2 %+ 1
    HASH 1
    RESET %3, %4
    %1 %2 %+ 0, [%4]
    HASH 0
%endmacro

%macro IMM 5
%assign dest 0
%rep 3
    RESET %3, %4
    %1 %2 %+ dest, %2 %+ 1, %2 %+ 2, %5
    HASH dest
%assign dest dest + 1
%endrep
    RESET %3, %4
    %1 %2 %+ 0, %2 %+ 1, [%4], %5
    HASH 0
%endmacro

%macro CVT 4
    RESET %4, %4
    %1 %2 %+ 0, %3 %+ 1
    HASH 0
    RESET %4, %4
    %1 %2 %+ 1, %3 %+ 1
    HASH 1
    RESET %4, %4
%ifidni %3, ymm
    %1 %2 %+ 0, yword [%4]
%elifidni %1, vcvtps2pd
%ifidni %2, xmm
    %1 %2 %+ 0, qword [%4]
%else
    %1 %2 %+ 0, oword [%4]
%endif
%elifidni %1, vcvtdq2pd
%ifidni %2, xmm
    %1 %2 %+ 0, qword [%4]
%else
    %1 %2 %+ 0, oword [%4]
%endif
%else
    %1 %2 %+ 0, oword [%4]
%endif
    HASH 0
%endmacro

%macro GROUP 3
%ifdef RECORD
%if __BITS__ = 64
    mov eax, 1
    mov edi, 1
    lea rsi, [digest]
    mov edx, 4
    syscall
%else
    mov eax, 4
    mov ebx, 1
    mov ecx, digest
    mov edx, 4
    int 0x80
%endif
%else
    mov ebx, %1
%if __BITS__ = 64
    cmp dword [digest], %2
%else
    cmp dword [digest], %3
%endif
    jne exit
%endif
    mov dword [digest], 2166136261
%endmacro

%macro PACKED 3
    BIN %1, xmm, %2, %3
    BIN %1, ymm, %2, %3
%endmacro
%macro SCALAR 2
    BIN %1, xmm, %2 %+ _a, %2 %+ _b
%endmacro
%macro FMA 1
    BIN %1 %+ ss, xmm, ps_a, ps_b
    BIN %1 %+ sd, xmm, pd_a, pd_b
    PACKED %1 %+ ps, ps_a, ps_b
    PACKED %1 %+ pd, pd_a, pd_b
%endmacro

section .text
_start:
    ldmxcsr [mxcsr]
    mov dword [digest], 2166136261

%if CASE_GROUP == 0 || CASE_GROUP == 1
    ; Packed arithmetic, logic, shuffle, horizontal operations: both widths.
    PACKED vaddpd, pd_a, pd_b
    PACKED vsubpd, pd_a, pd_b
    PACKED vmulpd, pd_a, pd_b
    PACKED vdivpd, pd_a, pd_b
    PACKED vhaddpd, pd_a, pd_b
    PACKED vhaddps, ps_a, ps_b
    PACKED vhsubpd, pd_a, pd_b
    PACKED vhsubps, ps_a, ps_b
    PACKED vandnpd, pd_a, pd_b
    PACKED vorps, ps_a, ps_b
    PACKED vxorps, ps_a, ps_b
    PACKED vunpckhpd, pd_a, pd_b
    PACKED vunpckhps, ps_a, ps_b
    PACKED vunpcklpd, pd_a, pd_b
    PACKED vunpcklps, ps_a, ps_b
    PACKED vpandn, ps_a, ps_b
    UNARY vsqrtpd, xmm, pd_a, pd_b
    UNARY vsqrtpd, ymm, pd_a, pd_b
    UNARY vsqrtps, xmm, ps_a, ps_b
    UNARY vsqrtps, ymm, ps_a, ps_b
    IMM vshufps, xmm, ps_a, ps_b, 0x1b
    IMM vshufps, ymm, ps_a, ps_b, 0x1b
    IMM vpblendd, xmm, ps_a, ps_b, 0
    IMM vpblendd, xmm, ps_a, ps_b, 15
    IMM vpblendd, xmm, ps_a, ps_b, 5
    IMM vpblendd, ymm, ps_a, ps_b, 0xa5
    GROUP 1, 0x80804666, 0x80804666
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 2
    ; Scalar results retain the rest of src1's XMM, and zero YMM high bits.
    SCALAR vaddsd, pd
    SCALAR vaddss, ps
    SCALAR vsubsd, pd
    SCALAR vsubss, ps
    SCALAR vmulsd, pd
    SCALAR vmulss, ps
    SCALAR vdivsd, pd
    SCALAR vdivss, ps
    SCALAR vsqrtsd, pd
    SCALAR vminsd, pd
    SCALAR vminss, ps
    SCALAR vmaxsd, pd
    SCALAR vmaxss, ps
    ; Infinity gives exact zero: approximation accuracy is outside this test.
    BIN vrcpss, xmm, positive_inf, positive_inf
    BIN vrsqrtss, xmm, positive_inf, positive_inf
%assign imm 0
%rep 16
    IMM vroundss, xmm, ps_a, fractional_ps, imm
    IMM vroundsd, xmm, pd_a, fractional_pd, imm
%assign imm imm + 1
%endrep
    GROUP 2, 0x87c9f9a5, 0x87c9f9a5
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 3
    ; Every comparison predicate, packed/scalar, aliases and NaN memory input.
%assign pred 0
%rep 32
    IMM vcmpps, xmm, ps_a, compare_ps, pred
    IMM vcmpps, ymm, ps_a, compare_ps, pred
    IMM vcmppd, xmm, pd_a, compare_pd, pred
    IMM vcmppd, ymm, pd_a, compare_pd, pred
    IMM vcmpss, xmm, ps_a, compare_ps, pred
    IMM vcmpsd, xmm, pd_a, compare_pd, pred
    RESET ps_a, qnan_ps
    vcmpps xmm0, xmm1, [qnan_ps], pred
    HASH 0
    RESET pd_a, qnan_pd
    vcmpsd xmm0, xmm1, [qnan_pd], pred
    HASH 0
%assign pred pred + 1
%endrep
    GROUP 3, 0x2b196285, 0x2b196285
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 4
    ; Integer comparisons are element-wise for XMM and YMM.
%macro ICMP 1
    PACKED %1, ints_a, ints_b
%endmacro
    ICMP vpcmpeqb
    ICMP vpcmpeqw
    ICMP vpcmpeqd
    ICMP vpcmpeqq
    ICMP vpcmpgtb
    ICMP vpcmpgtw
    ICMP vpcmpgtd
    ICMP vpcmpgtq
    GROUP 4, 0xc4edae5, 0xc4edae5
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 5
    ; Widening and narrowing conversion destinations, including YMM sources.
    CVT vcvtps2pd, xmm, xmm, ps_a
    CVT vcvtps2pd, ymm, xmm, ps_a
    CVT vcvtpd2ps, xmm, xmm, pd_a
    CVT vcvtpd2ps, xmm, ymm, pd_a
    CVT vcvtdq2ps, xmm, xmm, ints_a
    CVT vcvtdq2ps, ymm, ymm, ints_a
    CVT vcvtps2dq, xmm, xmm, ps_a
    CVT vcvtps2dq, ymm, ymm, ps_a
    CVT vcvttps2dq, xmm, xmm, ps_a
    CVT vcvttps2dq, ymm, ymm, ps_a
    CVT vcvtdq2pd, xmm, xmm, ints_a
    CVT vcvtdq2pd, ymm, xmm, ints_a
    CVT vcvtpd2dq, xmm, xmm, pd_a
    CVT vcvtpd2dq, xmm, ymm, pd_a
    CVT vcvttpd2dq, xmm, xmm, pd_a
    CVT vcvttpd2dq, xmm, ymm, pd_a
    BIN vcvtsd2ss, xmm, ps_a, pd_b
    BIN vcvtss2sd, xmm, pd_a, ps_b
%assign dest 0
%rep 2
    RESET pd_a, pd_b
    mov eax, -37
    vcvtsi2sd xmm %+ dest, xmm1, eax
    HASH dest
    RESET ps_a, ps_b
    mov eax, -37
    vcvtsi2ss xmm %+ dest, xmm1, eax
    HASH dest
%assign dest dest + 1
%endrep
    GROUP 5, 0x3a5ef3f7, 0x3a5ef3f7
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 6
    ; Duplicate/move/scalar merges and a legacy reduction preserving YMM high.
    UNARY vmovsldup, xmm, ps_a, ps_b
    UNARY vmovsldup, ymm, ps_a, ps_b
    UNARY vmovddup, xmm, pd_a, pd_b
    UNARY vmovddup, ymm, pd_a, pd_b
    BIN vmovss, xmm, ps_a, ps_b
    BIN vmovsd, xmm, pd_a, pd_b
    RESET ps_a, ps_b
    vmovss xmm0, [ps_b]
    HASH 0
    RESET pd_a, pd_b
    vmovsd xmm0, [pd_b]
    HASH 0
    RESET pd_a, pd_b
    vmovlpd xmm0, xmm1, [pd_b]
    HASH 0
    RESET ints_a, ints_b
    mov eax, 0x12345678
    vmovd xmm0, eax
    HASH 0
    RESET ints_a, ints_b
    vpbroadcastq xmm0, xmm1
    HASH 0
    RESET ints_a, ints_b
    vpbroadcastq ymm0, xmm1
    HASH 0
%assign imm 0
%rep 4
    RESET ints_a, ints_b
    vpshufd xmm0, xmm1, (imm * 0x55)
    HASH 0
    RESET ints_a, ints_b
    vpshufd ymm1, ymm1, (imm * 0x55)
    HASH 1
%assign imm imm + 1
%endrep
    RESET ints_a, ints_b
    phminposuw xmm0, xmm1
    HASH 0
    RESET ints_a, ints_b
    vphminposuw xmm0, xmm1
    HASH 0
    GROUP 6, 0xf0f8189d, 0xf0f8189d
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 7
    ; FMA operand orders and packed negative FMA, with both result widths.
    FMA vfmadd132
    FMA vfmadd213
    FMA vfmadd231
    FMA vfmsub132
    FMA vfmsub213
    FMA vfmsub231
    FMA vfnmadd132
    FMA vfnmadd213
    FMA vfnmadd231
    FMA vfnmsub132
    FMA vfnmsub213
    FMA vfnmsub231
    GROUP 7, 0x3b9bfec5, 0x3b9bfec5
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 8
    ; Blends, carryless multiply and full-width copies that must stay LASX.
    RESET pd_a, pd_b
    vblendvpd xmm0, xmm1, xmm2, xmm3
    HASH 0
    RESET pd_a, pd_b
    vblendvpd ymm0, ymm1, ymm2, ymm3
    HASH 0
    IMM vblendpd, xmm, pd_a, pd_b, 1
    IMM vblendpd, ymm, pd_a, pd_b, 5
    IMM vpclmulqdq, xmm, ints_a, ints_b, 0x11
    IMM vpclmulqdq, ymm, ints_a, ints_b, 0
    IMM vpclmulqdq, ymm, ints_a, ints_b, 0x11
    RESET ints_a, ints_b
    vmovdqu ymm0, ymm2
    HASH 0
    RESET ints_a, ints_b
    vinserti128 ymm0, ymm1, xmm2, 1
    HASH 0
    RESET pd_a, pd_b
    vinsertf128 ymm0, ymm1, xmm2, 1
    HASH 0
    RESET ints_a, ints_b
    vextracti128 xmm0, ymm2, 1
    HASH 0
    GROUP 8, 0x5aa54bd5, 0x5aa54bd5
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 9
    ; Qword-index/dword-result gather merges inactive XMM elements.
    RESET ints_a, ones
    vmovdqu ymm1, [indices]
    lea ADDR, [gather_data]
    vpgatherqd xmm0, [ADDR + ymm1 * 4], xmm2
    HASH 0
    RESET ints_a, partial_mask
    vmovdqu ymm1, [indices]
    lea ADDR, [gather_data]
    vpgatherqd xmm0, [ADDR + ymm1 * 4], xmm2
    HASH 0
    RESET ints_a, ones
    vmovdqu ymm1, [dword_indices]
    lea ADDR, [gather_data]
    vpgatherdd ymm0, [ADDR + ymm1 * 4], ymm2
    HASH 0
    RESET ints_a, ones
    vmovdqu xmm1, [dword_indices]
    lea ADDR, [gather_data64]
    vpgatherdq ymm0, [ADDR + xmm1 * 8], ymm2
    HASH 0
    RESET ints_a, ones
    vmovdqu ymm1, [indices]
    lea ADDR, [gather_data64]
    vpgatherqq ymm0, [ADDR + ymm1 * 8], ymm2
    HASH 0
    GROUP 9, 0x5b1ef605, 0x5b1ef605
%endif

%if CASE_GROUP == 0 || CASE_GROUP == 10
    ; Retained LASX path: also exposes a pre-existing model-only timeout.
    IMM vmpsadbw, xmm, ints_a, ints_b, 5
    GROUP 10, 0xc02d4635, 0xc02d4635
%endif

    xor ebx, ebx
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
ps_a: dd 4.0, 16.0, 64.0, 256.0, 0.5, 2.0, 8.0, 32.0
ps_b: dd 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0
pd_a: dq 4.0, 16.0, 64.0, 256.0
pd_b: dq 1.0, 2.0, 4.0, 8.0
compare_ps: dd 1.0, 16.0, 128.0, 256.0, 2.0, 2.0, 16.0, 32.0
compare_pd: dq 1.0, 16.0, 128.0, 256.0
qnan_ps: dd 0x7fc00000, 16.0, 128.0, 256.0, 0x7fc00000, 2.0, 16.0, 32.0
qnan_pd: dq 0x7ff8000000000000, 16.0, 128.0, 256.0
positive_inf: dd 0x7f800000, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0
fractional_ps: dd 1.25, -1.25, 2.75, -2.75, 1.25, -1.25, 2.75, -2.75
fractional_pd: dq 1.25, -1.25, 2.75, -2.75
ints_a: dd -16, 32, 64, -128, 256, -512, 1024, -2048
ints_b: dd 32, -32, 64, 0x80000000, -256, 512, 1024, 2048
ones: times 8 dd -1
indices: dq 0, 1, 2, 3
dword_indices: dd 0, 1, 2, 3, 4, 5, 6, 7
gather_data: dd 0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00, 5, 6, 7, 8
gather_data64: dq 0x1122334455667788, 0x99aabbccddeeff00, -1, 0x123456789abcdef
partial_mask: dd -1, 0, -1, 0, 0, 0, 0, 0
mxcsr: dd 0x1f80

section .bss align=32
actual: resb 32
digest: resd 1
section .note.GNU-stack noalloc noexec nowrite progbits
