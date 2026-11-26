#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise CMPXCHG16B code generation and the real signal interpreter bodies.

The signal context, page relocation and flags emitter are modelled. This is
not evidence of host signal delivery or concurrent 128-bit atomicity.
"""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]


def function(path, signature):
    source = (ROOT / path).read_text()
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


PRELUDE = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "latx-atomic.h"
#define CONFIG_LATX 1
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define LOCKINT_OK 0
#define LOCKINT_BAD 1
#define LOCKINT_SEGV 2
#define PAGE_WRITE_ORG 16
#define qemu_log_mask(...) ((void)0)
#define zero_ir2_opnd 0
#define UC_PC(c) ((c)->pc)
#define UC_GR(c) ((c)->regs)
#define ucontext_t Context
typedef uintptr_t abi_ulong;
typedef struct { uint64_t regs[32]; uintptr_t pc; } Context;
typedef siginfo_t Info;
static uint64_t qemu_host_page_mask = ~(uint64_t)4095;
static unsigned char memory[8192] __attribute__((aligned(4096)));
static int reg_gpr_map[] = {8,9,10,11};
enum { eax_index,ecx_index,edx_index,ebx_index };
int option_unaligned_atomic;
static int writable=1, restored, scq, scq_called;
static uint64_t checked_address, shadow_address;
static int page_get_flags(uint64_t a) { (void)a; return writable ? PAGE_WRITE_ORG : 0; }
static void *page_get_target_data(uint64_t a) { checked_address=a;return a==shadow_address && a ? memory : NULL; }
static void *set_interpret_newpage(uintptr_t page,int n) { CHECK(n==1); return (void *)page; }
static int recover_interpret_oldpage(uintptr_t page,void *p,int n) { CHECK(page==(uintptr_t)p && n==1);restored++;return 0; }
#define interpret_cmpxchg8b(uc, inst, addr) (*(addr)=0, LOCKINT_BAD)
#define interpret_sub(uc, inst, addr) (*(addr)=0, LOCKINT_BAD)
#define interpret_xchg(uc, inst, addr, size) (*(addr)=0, LOCKINT_BAD)
#define interpret_add(uc, inst, addr, size) (*(addr)=0, LOCKINT_BAD)
#define interpret_and(uc, inst, addr, size) (*(addr)=0, LOCKINT_BAD)
#define interpret_or(uc, inst, addr, size) (*(addr)=0, LOCKINT_BAD)
#define interpret_xor(uc, inst, addr, size) (*(addr)=0, LOCKINT_BAD)
#define interpret_cas(uc, inst, addr, size) (*(addr)=0, LOCKINT_BAD)
typedef int IR2_OPND;
typedef struct { int dummy; } IR1_OPND;
typedef struct { IR1_OPND opnd; } IR1_INST;
static uint32_t emitted[8];
static int emitted_count;
static int have_scq(void) { return scq; }
static bool translate_cmpxchg16b_scq(IR1_INST *i) { (void)i;scq_called++;return true; }
static IR1_OPND *ir1_get_opnd(IR1_INST *i,int n) { CHECK(n==0);return &i->opnd; }
static IR2_OPND convert_mem_no_offset(IR1_OPND *i) { (void)i;return 5; }
static IR2_OPND ra_alloc_itemp(void) { return 12; }
static void ra_free_temp(IR2_OPND r) { CHECK(r==12); }
static void la_break(int code) { emitted[emitted_count++]=0x002a0000 | code; }
static void la_ll_d(int a,int b,int off) { CHECK(!a && !b && off==4);emitted[emitted_count++]=0x22000400; }
static void la_amswap_d(int a,int b,int c) { emitted[emitted_count++]=0x38608000 | a | (b<<10) | (c<<5); }
static void generate_eflag_calculation(int a,int b,int c,IR1_INST *i,bool f) { (void)i;CHECK(a==12 && b==a && c==a && f); }
'''
MAIN = r'''
int main(void) {
    IR1_INST i={0};
    for(int mode=0;mode<2;mode++) {
        option_unaligned_atomic=mode;emitted_count=scq_called=0;scq=0;
        CHECK(translate_cmpxchg16b(&i));
        CHECK(emitted_count==2 && !scq_called);
        CHECK(emitted[0]==(mode ? LATX_CMPXCHG16B_BREAK : 0x22000400));
        for(int equal=0;equal<2;equal++) {
            uint64_t *p=(uint64_t *)(memory+64);p[0]=17;p[1]=19;
            Context uc={.pc=(uintptr_t)emitted};
            Info info={.si_signo=mode ? SIGTRAP : SIGBUS,
                       .si_addr=mode ? (void *)0x1234 : p};
            uc.regs[5]=(uintptr_t)p;
            uc.regs[8]=equal ? 17 : 0;uc.regs[10]=equal ? 19 : 0;
            uc.regs[11]=23;uc.regs[9]=29;
            writable=1;restored=0;
            CHECK(lock_interpret(&info,&uc)==0 && restored==1);
            CHECK(checked_address==(uintptr_t)p);
            CHECK(uc.pc==(uintptr_t)(emitted+2) && uc.regs[12]==(equal ? 0 : 1));
            CHECK(p[0]==(equal ? 23 : 17) && p[1]==(equal ? 29 : 19));
            if(!equal) CHECK(uc.regs[8]==17 && uc.regs[10]==19);
        }
        Context uc={.pc=(uintptr_t)emitted};uc.regs[5]=(uintptr_t)(memory+64);
        Info info={.si_signo=mode ? SIGTRAP : SIGBUS};
        writable=0;restored=0;
        CHECK(lock_interpret(&info,&uc)==LOCKINT_SEGV && !restored);
        CHECK(info.si_signo==SIGSEGV && info.si_code==SEGV_ACCERR);
        CHECK(info.si_addr==memory+64 && uc.pc==(uintptr_t)(emitted+1));
        scq=1;emitted_count=0;
        CHECK(translate_cmpxchg16b(&i) && scq_called==1 && !emitted_count);
    }
    emitted[0]=LATX_CMPXCHG16B_BREAK;emitted[1]=0x38608000 | 12 | (5<<5);
    Context uc={.pc=(uintptr_t)emitted};Info info={0};
    option_unaligned_atomic=0;CHECK(lock_interpret(&info,&uc)==LOCKINT_BAD);
    option_unaligned_atomic=1;emitted[1]=0;CHECK(lock_interpret(&info,&uc)==LOCKINT_BAD);
    emitted[1]=0x38608000 | 12 | (5<<5);
    uc.regs[5]=(uintptr_t)(memory+64);shadow_address=uc.regs[5];
    info.si_addr=(void *)0x1234;
    CHECK(lock_interpret(&info,&uc)==LOCKINT_BAD && info.si_addr==memory+64);
    puts("PASS: legacy/new CMPXCHG16B generation, interpreter success/mismatch/permission and SC.Q selection");
    return 0;
}
'''
source = PRELUDE + '\n' + '\n'.join([
    function('target/i386/latx/translator/tr-mov.c', 'bool translate_cmpxchg16b('),
    function('accel/tcg/translate-all.c', 'int lock_interpret_check(uint64_t addr)\n{'),
    function('accel/tcg/translate-all.c', 'static int interpret_cmpxchg16b(ucontext_t *uc, uint32_t* inst,\n'),
    function('accel/tcg/translate-all.c', 'static void lock_interpret_segv('),
    function('accel/tcg/translate-all.c', 'int lock_interpret(siginfo_t *info, ucontext_t *uc)'),
]) + MAIN
with tempfile.TemporaryDirectory(prefix='latx-atomic-trap-') as d:
    p = Path(d)
    (p / 'test.c').write_text(source)
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-I' + str(ROOT / 'target/i386/latx/include'), str(p / 'test.c'), '-o', str(p / 'test')], check=True)
    subprocess.run([str(p / 'test')], check=True)
