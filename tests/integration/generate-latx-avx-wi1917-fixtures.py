#!/usr/bin/env python3
"""Generate one independent xzy86 fixture for each AVX gather mnemonic."""

from pathlib import Path

INTEGRATION = Path(__file__).resolve().parent
MNEMONICS = "vpgatherdd vpgatherdq vpgatherqd vpgatherqq vgatherdps vgatherdpd vgatherqps vgatherqpd".split()
XMM_ONLY = {"vpgatherqd", "vgatherqps"}
INDEX_KIND = {
    "vpgatherdd": "d", "vpgatherdq": "d", "vpgatherqd": "q", "vpgatherqq": "q",
    "vgatherdps": "d", "vgatherdpd": "d", "vgatherqps": "q", "vgatherqpd": "q",
}
DATA_KIND = {
    "vpgatherdd": "d", "vpgatherdq": "q", "vpgatherqd": "d", "vpgatherqq": "q",
    "vgatherdps": "d", "vgatherdpd": "q", "vgatherqps": "d", "vgatherqpd": "q",
}

INDEX_WIDTH = {
    "vpgatherdq": "xmm", "vgatherdpd": "xmm",
}


def save_macro(m):
    return f"""    .macro SAVE_{m.upper()} offset, dreg, mreg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + {m}_xsave_area]
    movdqu xmm14, xmmword ptr [rip + {m}_xsave_area + 160 + (\\dreg) * 16]
    movdqu xmmword ptr [rdi + \\offset], xmm14
    movdqu xmm14, xmmword ptr [rip + {m}_xsave_area + 576 + (\\dreg) * 16]
    movdqu xmmword ptr [rdi + \\offset + 16], xmm14
    movdqu xmm14, xmmword ptr [rip + {m}_xsave_area + 160 + (\\mreg) * 16]
    movdqu xmmword ptr [rdi + \\offset + 32], xmm14
    movdqu xmm14, xmmword ptr [rip + {m}_xsave_area + 576 + (\\mreg) * 16]
    movdqu xmmword ptr [rdi + \\offset + 48], xmm14
    .endm
"""


def load(width, reg, label):
    word = "ymmword" if width == "ymm" else "xmmword"
    return f"    vmovdqu {width}{reg}, {word} ptr [rip + {label}]"


def index_width(m, width):
    return INDEX_WIDTH.get(m, width)


def memory_operand(m, width):
    index = f"{index_width(m, width)}1"
    scale = "4" if INDEX_KIND[m] == "d" else "8"
    word = "qword" if DATA_KIND[m] == "q" else "dword"
    return f"{word} ptr [r8 + {index} * {scale}]"


def cases(m):
    body = []
    suffix = INDEX_KIND[m]
    widths = ("xmm",) if m in XMM_ONLY else ("xmm", "ymm")
    for index, width, indices, mask in [
        (0, "xmm", f"{m}_indices_pos_xmm_{suffix}", f"{m}_mask_all_xmm"),
        (1, "xmm", f"{m}_indices_neg_xmm_{suffix}", f"{m}_mask_partial_xmm"),
        (2, "ymm", f"{m}_indices_pos_ymm_{suffix}", f"{m}_mask_all_ymm"),
        (3, "ymm", f"{m}_indices_neg_ymm_{suffix}", f"{m}_mask_partial_ymm"),
    ][:len(widths) * 2]:
        index_width_value = index_width(m, width)
        body += [
            load("ymm", 0, f"{m}_dest_seed"),
            load(index_width_value, 1, indices),
            load(width, 2, mask),
            "    lea r8, [rip + gather_base]",
            f"    {m} {width}0, {memory_operand(m, width)}, {width}2",
            f"    SAVE_{m.upper()} {index * 64}, 0, 2",
        ]
    return body


def fault(m):
    width = "xmm" if m in XMM_ONLY else "ymm"
    return f"    {m} {width}0, {memory_operand(m, width).replace('r8', 'rsi')}, {width}2"


def assembly(m):
    operation_width = "xmm" if m in XMM_ONLY else "ymm"
    idx_width = index_width(m, operation_width)
    suffix = INDEX_KIND[m]
    body = ["    test rsi, rsi", "    jnz .Lfault", *cases(m), "    ret", ".Lfault:",
            load(idx_width, 1, f"{m}_indices_zero_{idx_width}_{suffix}"), load(operation_width, 2, f"{m}_mask_all_{operation_width}"),
            fault(m), "    ret"]
    return f"""/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
{save_macro(m)}
    .globl latx_avx_single_{m}_run
    .type latx_avx_single_{m}_run, @function
latx_avx_single_{m}_run:
{chr(10).join(body)}

    .section .data
    .balign 32
{m}_dest_seed: .zero 32
{m}_indices_pos_xmm_d: .long 0,2,4,6
{m}_indices_neg_xmm_d: .long -1,-2,-3,-4
{m}_indices_pos_ymm_d: .long 0,2,4,6,8,10,12,14
{m}_indices_neg_ymm_d: .long -1,-2,-3,-4,-5,-6,-7,-8
{m}_indices_zero_xmm_d: .long 0,0,0,0
{m}_indices_zero_ymm_d: .long 0,0,0,0,0,0,0,0
{m}_mask_all_xmm: .long -1,-1,-1,-1
{m}_mask_partial_xmm: .long -1,0,-1,0
{m}_mask_all_ymm: .long -1,-1,-1,-1,-1,-1,-1,-1
{m}_mask_partial_ymm: .long -1,0,-1,0,-1,0,-1,0
    .balign 8
{m}_indices_pos_xmm_q: .quad 0,1
{m}_indices_neg_xmm_q: .quad -1,-2
{m}_indices_zero_xmm_q: .quad 0,0
{m}_indices_pos_ymm_q: .quad 0,1,2,3
{m}_indices_neg_ymm_q: .quad -1,-2,-3,-4
{m}_indices_zero_ymm_q: .quad 0,0,0,0
    .balign 64
{m}_xsave_area: .zero 4096
    .balign 32
gather_table:
    .rept 16
    .quad 0x3ff0000000000001
    .endr
gather_base:
    .rept 16
    .quad 0x3ff0000000000001
    .endr
""".replace(f"{m}_indices_pos_xmm,", f"{m}_indices_pos_xmm,")


def c_source(m):
    output_size = 128 if m in XMM_ONLY else 256
    return f"""/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"
extern void latx_avx_single_{m}_run(uint8_t *, uint8_t *);
static long syscall6(long n,long a,long b,long c,long d,long e,long f) {{
    register long r10 __asm__("r10")=d, r8 __asm__("r8")=e, r9 __asm__("r9")=f, rax __asm__("rax")=n;
    register long rdi __asm__("rdi")=a, rsi __asm__("rsi")=b, rdx __asm__("rdx")=c;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi),"S"(rsi),"d"(rdx),"r"(r10),"r"(r8),"r"(r9) : "rcx","r11","memory");
    return rax;
}}
static uint8_t output[{output_size}];
int latx_avx_single_main(long argc, char **argv) {{
    uint8_t *fault_ptr = 0;
    if (argc > 1 && argv[1][0] == 'f') {{
        long map = syscall6(9,0,8192,3,0x22,-1,0);
        if (map < 0 || syscall6(10,map+4096,4096,0,0,0,0) < 0) return 2;
        fault_ptr = (uint8_t *)(map + 4095);
    }}
    latx_avx_single_{m}_run(output, fault_ptr);
    if (fault_ptr) return 1;
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}}
"""


for mnemonic in MNEMONICS:
    text = assembly(mnemonic)
    (INTEGRATION / f"latx-avx-single-{mnemonic}.S").write_text(text)
    (INTEGRATION / f"latx-avx-single-{mnemonic}.c").write_text(c_source(mnemonic))
print("generated=8")
print("mnemonics=" + " ".join(MNEMONICS))
