#!/usr/bin/env python3
"""Generate one independent xzy86 fixture for each WI-1915 mnemonic."""

from pathlib import Path

INTEGRATION = Path(__file__).resolve().parent
THREE = "vpmuldq vpmulhrsw vpmulhuw vpmulhw vpmulld vpmullw vpmuludq vpavgb vpavgw vphaddd vphaddsw vphaddw vphsubd vphsubsw vphsubw vpmaddubsw vpmaddwd vpsadbw vpsignb vpsignd vpsignw".split()
UNARY = "vpabsb vpabsd vpabsw".split()
PMOV = {
    "vpmovsxbd": ("dword", "qword"), "vpmovsxbq": ("word", "dword"),
    "vpmovsxbw": ("qword", "xmmword"), "vpmovsxdq": ("qword", "xmmword"),
    "vpmovsxwd": ("qword", "xmmword"), "vpmovsxwq": ("dword", "qword"),
    "vpmovzxbd": ("dword", "qword"), "vpmovzxbq": ("word", "dword"),
    "vpmovzxbw": ("qword", "xmmword"), "vpmovzxdq": ("qword", "xmmword"),
    "vpmovzxwd": ("qword", "xmmword"), "vpmovzxwq": ("dword", "qword"),
}
VPHMIN = "vphminposuw"
MSK = "vpmovmskb"
ALL = sorted(THREE + UNARY + list(PMOV) + [VPHMIN, MSK])


def load(width, reg, label):
    word = "ymmword" if width == "ymm" else "xmmword"
    return f"    vmovdqu {width}{reg}, {word} ptr [rip + {label}]"


def save(m):
    return f"""    .macro SAVE_{m.upper()} offset, reg
    mov eax, 6
    xor edx, edx
    xsave64 [rip + {m}_xsave_area]
    movdqu xmm14, xmmword ptr [rip + {m}_xsave_area + 160 + (\\reg) * 16]
    movdqu xmmword ptr [rdi + \\offset], xmm14
    movdqu xmm14, xmmword ptr [rip + {m}_xsave_area + 576 + (\\reg) * 16]
    movdqu xmmword ptr [rdi + \\offset + 16], xmm14
    .endm
"""


def three(m):
    out = []
    index = 0
    for width in ("xmm", "ymm"):
        word = "ymmword" if width == "ymm" else "xmmword"
        for dest, source2 in ((2, f"{width}1"), (0, f"{width}1")):
            out += [load(width, 0, f"{m}_input_a"), load(width, 1, f"{m}_input_b"),
                    f"    {m} {width}{dest}, {width}0, {source2}",
                    f"    SAVE_{m.upper()} {index * 32}, {dest}"]
            index += 1
        for dest in (2, 0):
            out += [load(width, 0, f"{m}_input_a"),
                    f"    {m} {width}{dest}, {width}0, {word} ptr [rip + {m}_input_b]",
                    f"    SAVE_{m.upper()} {index * 32}, {dest}"]
            index += 1
    return out


def unary(m):
    out = []
    index = 0
    for width in ("xmm", "ymm"):
        word = "ymmword" if width == "ymm" else "xmmword"
        for operand in (f"{width}0", f"{word} ptr [rip + {m}_input_a]"):
            for dest in (2, 0):
                if operand.endswith("0"):
                    out.append(load(width, 0, f"{m}_input_a"))
                out += [f"    {m} {width}{dest}, {operand}",
                        f"    SAVE_{m.upper()} {index * 32}, {dest}"]
                index += 1
    return out


def pmov(m):
    xmm_word, ymm_word = PMOV[m]
    out = []
    cases = [
        ("xmm2", "xmm0"), ("xmm0", "xmm0"), ("xmm2", f"{xmm_word} ptr [rip + {m}_input_a]"),
        ("ymm2", "xmm0"), ("ymm0", "xmm0"), ("ymm2", f"{ymm_word} ptr [rip + {m}_input_a]"),
        ("xmm0", f"{xmm_word} ptr [rip + {m}_input_a]"), ("ymm0", f"{ymm_word} ptr [rip + {m}_input_a]"),
    ]
    for index, (dest, source) in enumerate(cases):
        if source == "xmm0":
            out.append(load("xmm", 0, f"{m}_input_a"))
        elif source.endswith("xmm0"):
            out.append(load("xmm", 0, f"{m}_input_a"))
        out += [f"    {m} {dest}, {source}",
                f"    SAVE_{m.upper()} {index * 32}, {dest[-1]}"]
    return out


def phmin():
    m = VPHMIN
    return [load("xmm", 0, f"{m}_input_a"), f"    {m} xmm2, xmm0", f"    SAVE_{m.upper()} 0, 2",
            load("ymm", 0, f"{m}_input_a"), f"    {m} xmm0, xmm0", f"    SAVE_{m.upper()} 32, 0",
            f"    {m} xmm2, xmmword ptr [rip + {m}_input_a]", f"    SAVE_{m.upper()} 64, 2",
            f"    {m} xmm0, xmmword ptr [rip + {m}_input_a]", f"    SAVE_{m.upper()} 96, 0"]


def movmsk():
    m = MSK
    out = []
    for index, width in enumerate(("xmm", "xmm", "xmm", "xmm", "ymm", "ymm", "ymm", "ymm")):
        reg = index & 1
        out += [load(width, reg, f"{m}_input_{'a' if index & 1 == 0 else 'b'}"),
                f"    {m} eax, {width}{reg}", f"    mov qword ptr [rdi + {index * 8}], rax"]
    return out


def fault(m):
    if m in THREE:
        return f"    {m} ymm2, ymm0, ymmword ptr [rsi]"
    if m in UNARY:
        return f"    {m} ymm2, ymmword ptr [rsi]"
    if m in PMOV:
        return f"    {m} ymm2, {PMOV[m][1]} ptr [rsi]"
    if m == VPHMIN:
        return f"    {m} xmm2, xmmword ptr [rsi]"
    return None


def assembly(m):
    if m in THREE:
        body, size, count = three(m), 256, 8
    elif m in UNARY:
        body, size, count = unary(m), 256, 8
    elif m in PMOV:
        body, size, count = pmov(m), 256, 8
    elif m == VPHMIN:
        body, size, count = phmin(), 128, 4
    else:
        body, size, count = movmsk(), 64, 8
    bad = fault(m)
    if bad:
        body = ["    test rsi, rsi", "    jnz .Lfault", *body, "    ret", ".Lfault:", bad, "    ret"]
    else:
        body = ["    test rsi, rsi", "    jnz .Lunsupported", *body, "    ret", ".Lunsupported:", "    mov eax, 2", "    ret"]
    data = f"""    .section .data
    .balign 32
{m}_input_a: .byte 0x80,0x7f,0x01,0xff,0x00,0x7e,0x02,0xfe,0x11,0xee,0x22,0xdd,0x33,0xcc,0x44,0xbb,0x55,0xaa,0x66,0x99,0x77,0x88,0x10,0xf0,0x20,0xe0,0x30,0xd0,0x40,0xc0,0x50,0xb0
{m}_input_b: .byte 0x7f,0x80,0xff,0x01,0x7e,0x82,0xfe,0x02,0xee,0x11,0xdd,0x22,0xcc,0x33,0xbb,0x44,0xaa,0x55,0x99,0x66,0x88,0x77,0xf0,0x10,0xe0,0x20,0xd0,0x30,0xc0,0x40,0xb0,0x50
    .balign 64
{m}_xsave_area: .zero 4096
"""
    return f"""/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
{save(m)}
    .globl latx_avx_single_{m}_run
    .type latx_avx_single_{m}_run, @function
latx_avx_single_{m}_run:
{chr(10).join(body)}

{data}
    .section .note.GNU-stack,"",@progbits
"""


def c_source(m):
    size = 64 if m == MSK else 128 if m == VPHMIN else 256
    return f"""/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"
extern void latx_avx_single_{m}_run(uint8_t *, uint8_t *);
static long syscall6(long n,long a,long b,long c,long d,long e,long f) {{
    register long r10 __asm__("r10")=d, r8 __asm__("r8")=e, r9 __asm__("r9")=f, rax __asm__("rax")=n;
    register long rdi __asm__("rdi")=a, rsi __asm__("rsi")=b, rdx __asm__("rdx")=c;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi),"S"(rsi),"d"(rdx),"r"(r10),"r"(r8),"r"(r9) : "rcx","r11","memory");
    return rax;
}}
static uint8_t output[{size}];
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


for mnemonic in ALL:
    (INTEGRATION / f"latx-avx-single-{mnemonic}.S").write_text(assembly(mnemonic))
    (INTEGRATION / f"latx-avx-single-{mnemonic}.c").write_text(c_source(mnemonic))
print(f"generated={len(ALL)}")
print("mnemonics=" + " ".join(ALL))
