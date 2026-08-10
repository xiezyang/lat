#!/usr/bin/env python3
"""Generate the independent xzy86 VPANDN fixture for WI-1794."""

from pathlib import Path
import argparse


ASM = r'''/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .macro SAVE_RESULT
    vmovdqu ymmword ptr [rdi], ymm2
    add rdi, 32
    .endm
    .globl latx_avx_single_vpandn_run
latx_avx_single_vpandn_run:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpandn xmm2, xmm0, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpandn xmm0, xmm0, xmm1
    vmovdqu xmm2, xmm0
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vmovdqu xmm1, xmmword ptr [rip + input_b]
    vpandn xmm1, xmm0, xmm1
    vmovdqu xmm2, xmm1
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpandn xmm0, xmm0, xmm0
    vmovdqu xmm2, xmm0
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpandn xmm2, xmm0, xmmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpandn xmm0, xmm0, xmmword ptr [rip + input_b_unaligned]
    vmovdqu xmm2, xmm0
    SAVE_RESULT

    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpandn ymm2, ymm0, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpandn ymm0, ymm0, ymm1
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vmovdqu ymm1, ymmword ptr [rip + input_b]
    vpandn ymm1, ymm0, ymm1
    vmovdqu ymm2, ymm1
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpandn ymm0, ymm0, ymm0
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpandn ymm2, ymm0, ymmword ptr [rip + input_b_unaligned]
    SAVE_RESULT
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpandn ymm0, ymm0, ymmword ptr [rip + input_b_unaligned]
    vmovdqu ymm2, ymm0
    SAVE_RESULT
    vzeroupper
    ret

    .globl latx_avx_single_vpandn_fault_xmm
latx_avx_single_vpandn_fault_xmm:
    vmovdqu xmm0, xmmword ptr [rip + input_a]
    vpandn xmm2, xmm0, xmmword ptr [rdi]
    ret
    .globl latx_avx_single_vpandn_fault_ymm
latx_avx_single_vpandn_fault_ymm:
    vmovdqu ymm0, ymmword ptr [rip + input_a]
    vpandn ymm2, ymm0, ymmword ptr [rdi]
    vzeroupper
    ret
    .globl latx_avx_single_rt_sigreturn
latx_avx_single_rt_sigreturn:
    mov eax, 15
    syscall

    .section .rodata
    .balign 32
input_a:
    .byte 0x00,0xff,0x0f,0xf0,0x55,0xaa,0x33,0xcc,0x80,0x7f,0x01,0xfe,0x12,0x34,0x56,0x78
    .byte 0x11,0xee,0x22,0xdd,0x44,0xbb,0x88,0x77,0x99,0x66,0xab,0xcd,0xde,0xad,0xbe,0xef
input_b_unaligned:
    .byte 0x5a
input_b:
    .byte 0xff,0x00,0xf0,0x0f,0xaa,0x55,0xcc,0x33,0x7f,0x80,0xfe,0x01,0x34,0x12,0x78,0x56
    .byte 0xee,0x11,0xdd,0x22,0xbb,0x44,0x77,0x88,0x66,0x99,0xcd,0xab,0xad,0xde,0xef,0xbe
    .section .note.GNU-stack,"",@progbits
'''


C = r'''/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"
enum { SYS_MMAP=9, SYS_MPROTECT=10, SYS_RT_SIGACTION=13, SYS_EXIT=60,
       PROT_NONE=0, PROT_READ=1, PROT_WRITE=2, MAP_PRIVATE=2,
       MAP_ANONYMOUS=0x20, SA_SIGINFO=4, SA_RESTORER=0x04000000,
       SIGBUS=7, SIGSEGV=11, PAGE=4096, PAGES=2, OUTPUT_SIZE=384 };
struct kernel_sigaction { void (*handler)(int,void *,void *); unsigned long flags;
    void (*restorer)(void); unsigned long mask; };
extern void latx_avx_single_vpandn_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vpandn_fault_xmm(uint8_t *);
extern void latx_avx_single_vpandn_fault_ymm(uint8_t *);
extern void latx_avx_single_rt_sigreturn(void);
static uint8_t output[OUTPUT_SIZE], fault_capture[16];
static uint8_t *fault_target; static const char *fault_name;
static inline long syscall3(long n,long a,long b,long c) { register long rax __asm__("rax")=n;
    register long rdi __asm__("rdi")=a, rsi __asm__("rsi")=b, rdx __asm__("rdx")=c;
    __asm__ volatile("syscall": "+a"(rax): "D"(rdi),"S"(rsi),"d"(rdx):"rcx","r11","memory"); return rax; }
static inline long syscall4(long n,long a,long b,long c,long d) { register long rax __asm__("rax")=n;
    register long rdi __asm__("rdi")=a, rsi __asm__("rsi")=b, rdx __asm__("rdx")=c, r10 __asm__("r10")=d;
    __asm__ volatile("syscall": "+a"(rax): "D"(rdi),"S"(rsi),"d"(rdx),"r"(r10):"rcx","r11","memory"); return rax; }
static inline long syscall6(long n,long a,long b,long c,long d,long e,long f) { register long rax __asm__("rax")=n;
    register long rdi __asm__("rdi")=a, rsi __asm__("rsi")=b, rdx __asm__("rdx")=c, r10 __asm__("r10")=d, r8 __asm__("r8")=e, r9 __asm__("r9")=f;
    __asm__ volatile("syscall": "+a"(rax): "D"(rdi),"S"(rsi),"d"(rdx),"r"(r10),"r"(r8),"r"(r9):"rcx","r11","memory"); return rax; }
static __attribute__((noreturn)) void exit_now(int status) { __asm__ volatile("syscall"::"a"((long)SYS_EXIT),"D"((long)status):"rcx","r11","memory"); __builtin_unreachable(); }
static int same(const char *a,const char *b) { while (*a && *a==*b) { ++a; ++b; } return *a==*b; }
static void handler(int sig,void *info,void *ctx) { (void)info; (void)ctx; for(unsigned i=0;i<8;++i) fault_capture[i]=fault_name[i]?(uint8_t)fault_name[i]:0; for(unsigned i=0;i<8;++i) fault_capture[8+i]=fault_target[i]; latx_avx_single_write_all(fault_capture,sizeof(fault_capture)); exit_now(128+sig); }
static int install_handlers(void) { struct kernel_sigaction a={handler,SA_SIGINFO|SA_RESTORER,latx_avx_single_rt_sigreturn,0}; return syscall4(SYS_RT_SIGACTION,SIGSEGV,(long)(uintptr_t)&a,0,8)<0 || syscall4(SYS_RT_SIGACTION,SIGBUS,(long)(uintptr_t)&a,0,8)<0; }
int latx_avx_single_main(long argc,char **argv) { long mapping; uint8_t *page;
    if(argc==1) { latx_avx_single_vpandn_run(output,0); return latx_avx_single_write_all(output,sizeof(output))!=0; }
    if(argc!=2 || (!same(argv[1],"xmm-cross-8") && !same(argv[1],"ymm-cross-16"))) return 2;
    mapping=syscall6(SYS_MMAP,0,PAGES*PAGE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0); if(mapping<0) return 3;
    page=(uint8_t *)(uintptr_t)mapping; latx_avx_single_fill(page,PAGES*PAGE,UINT64_C(0x3d8a1f52c7e9046b));
    if(install_handlers() || syscall3(SYS_MPROTECT,(long)(uintptr_t)(page+PAGE),PAGE,PROT_NONE)<0) return 4;
    if(same(argv[1],"xmm-cross-8")) { fault_name="xmm-cross-8"; fault_target=page+PAGE-8; latx_avx_single_vpandn_fault_xmm(fault_target); }
    else { fault_name="ymm-cross-16"; fault_target=page+PAGE-16; latx_avx_single_vpandn_fault_ymm(fault_target); }
    return 5;
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "latx-avx-single-vpandn.S").write_text(ASM, encoding="ascii")
    (args.output_dir / "latx-avx-single-vpandn.c").write_text(C, encoding="ascii")
    print("mnemonic=vpandn cases=12")


if __name__ == "__main__":
    main()
