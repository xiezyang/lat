#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Execute emitted carry/NEG IR with a byte-addressed, conflict-injecting model.

This checks production code generation, not LoongArch hardware or the LBT flag
instructions. The integration guest independently checks x86-visible flags.
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
#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define SIGN_EXTENSION 1
#define PAGE_WRITE 2
#define PAGE_WRITE_ORG 16
#define dt_X86_INS_ADC 1
#define dt_X86_INS_SBB 2
#define dt_X86_INS_NEG 3
#define dt_X86_INS_BTS 4
#define dt_X86_INS_BTR 5
#define dt_X86_INS_BTC 6
#define dt_X86_INS_BT 7
#define lsassertm(x, ...) CHECK(x)
#define zero_ir2_opnd 0
#define error_report(...) fprintf(stderr, __VA_ARGS__)
static int option_unaligned_atomic;
static int capability = 1;
static int have_am(void) { return capability; }
typedef int IR2_OPND;
typedef int IR2_INST;
typedef int IR1_OPCODE;
typedef struct { int size; } IR1_OPND;
typedef struct { IR1_OPND opnd[2]; int opcode; } IR1_INST;
static IR1_OPND *ir1_get_opnd(IR1_INST *i, int n) { return &i->opnd[n]; }
static int ir1_opnd_size(IR1_OPND *o) { return o->size; }
static int ir1_opcode(IR1_INST *i) { return i->opcode; }
static IR2_OPND convert_mem_no_offset(IR1_OPND *o) { (void)o; return 1; }
static IR2_OPND load_ireg_from_ir1(IR1_OPND *o, int e, bool b) { (void)o;(void)e;(void)b;return 2; }
static int next_reg, next_label, count, carry, injected, conflict, flags_seen, live_temps;
static unsigned char memory[192], before[192];
static uint64_t regs[256];
enum { ADC, SBC, ADD, SUB, OR, LOAD, AMADD, CAS, LABEL, BNE, FLAGS, PAGE,
       M_AND, M_XOR, M_NOR, SLL, IMM, AMAND, AMOR, AMXOR };
struct insn { int op,a,b,c,size; } code[128];
static IR2_INST *emit(int op,int a,int b,int c,int size) {
    CHECK(count < 128); code[count++] = (struct insn){op,a,b,c,size}; return NULL;
}
static IR2_OPND ra_alloc_itemp(void) { CHECK(++live_temps <= 7); return next_reg++; }
static IR2_OPND ra_alloc_label(void) { return next_label++; }
static void ra_free_temp(int r) { CHECK(r > 0 && live_temps > 0); live_temps--; }
#define BIN(name, op) static IR2_INST *name(int a,int b,int c) { return emit(op,a,b,c,0); }
BIN(la_adc_d, ADC) BIN(la_sbc_d, SBC) BIN(la_add_d, ADD)
BIN(la_sub_d, SUB) BIN(la_or, OR) BIN(la_bne, BNE)
BIN(la_and, M_AND) BIN(la_xor, M_XOR) BIN(la_nor, M_NOR) BIN(la_sll_d, SLL)
static void la_ori(int a,int b,int imm) { CHECK(b==0);emit(IMM,a,0,imm,0); }
static void get_mem_bitbase(int r,int *off,IR1_OPND *a,IR1_OPND *b,bool llsc) {
    (void)a;(void)b;CHECK(llsc);*off=0;la_or(r,1,0);
    /* Model the signed narrow index temporary retained by this helper. */
    ra_alloc_itemp();
}
static void get_bitoffset(int r,IR1_OPND *a,IR1_OPND *b) { (void)a;(void)b;la_or(r,2,0); }
static void la_label(int a) { emit(LABEL,a,0,0,0); }
static void gen_test_page_flag(int a,int off,int flags,int bytes) {
    CHECK(off == 0 && flags == (PAGE_WRITE | PAGE_WRITE_ORG));
    /* The real page check needs three simultaneous integer temporaries. */
    int t0=ra_alloc_itemp(), t1=ra_alloc_itemp(), t2=ra_alloc_itemp();
    ra_free_temp(t2);ra_free_temp(t1);ra_free_temp(t0);
    emit(PAGE,a,0,0,bytes);
}
static void load_ireg_from_ir2_mem_2(int a,int b,int off,int size,int ext,bool hi) {
    CHECK(off == 0 && ext == SIGN_EXTENSION && !hi); emit(LOAD,a,b,0,size);
}
static void generate_eflag_calculation(int a,int b,int c,IR1_INST *i,bool f) {
    CHECK(f); emit(FLAGS,a,b,c,i->opcode);
}
#define ATOM(suffix, bits) \
static IR2_INST *la_amadd_db_##suffix(int a,int b,int c) { return emit(AMADD,a,b,c,bits); } \
static IR2_INST *la_amcas_db_##suffix(int a,int b,int c) { return emit(CAS,a,b,c,bits); }
ATOM(b,8) ATOM(h,16) ATOM(w,32) ATOM(d,64)
#define LOGIC(s,bits) \
static IR2_INST *la_amand_db_##s(int a,int b,int c) { return emit(AMAND,a,b,c,bits); } \
static IR2_INST *la_amor_db_##s(int a,int b,int c) { return emit(AMOR,a,b,c,bits); } \
static IR2_INST *la_amxor_db_##s(int a,int b,int c) { return emit(AMXOR,a,b,c,bits); }
LOGIC(w,32) LOGIC(d,64)
#define ATO_AMADD(s) ((s)==64 ? la_amadd_db_d : (s)==32 ? la_amadd_db_w : (s)==16 ? la_amadd_db_h : la_amadd_db_b)
#define ATO_AMCAS(s) ((s)==64 ? la_amcas_db_d : (s)==32 ? la_amcas_db_w : (s)==16 ? la_amcas_db_h : la_amcas_db_b)
static uint64_t mask(int size) { return size == 64 ? UINT64_MAX : (UINT64_C(1)<<size)-1; }
static uint64_t sx(uint64_t x,int size) { return (uint64_t)((int64_t)(x << (64-size)) >> (64-size)); }
static uint64_t read_mem(uint64_t address,int size) {
    uint64_t x=0; memcpy(&x,(void *)(uintptr_t)address,size/8); return x;
}
static void write_mem(uint64_t address,int size,uint64_t value) {
    memcpy((void *)(uintptr_t)address,&value,size/8);
}
'''

MAIN = r'''
static void run(int size,int op,uint64_t old,uint64_t src,int cf,int offset,int clash) {
    memset(memory,0xa5,sizeof(memory));
    memset(regs,0,sizeof(regs)); carry=cf; injected=flags_seen=0; conflict=clash;
    regs[1]=(uintptr_t)(memory+64+offset); regs[2]=sx(src,size);
    write_mem(regs[1],size,old); memcpy(before,memory,sizeof(memory));
    uint64_t actual_old=old;
    int steps=0;
    for (int pc=0;pc<count;pc++) {
        struct insn i=code[pc]; uint64_t value;
        CHECK(++steps < 100);
        switch(i.op) {
        case ADC: regs[i.a]=regs[i.b]+regs[i.c]+carry;break;
        case SBC: regs[i.a]=regs[i.b]-regs[i.c]-carry;break;
        case ADD: regs[i.a]=regs[i.b]+regs[i.c];break;
        case SUB: regs[i.a]=regs[i.b]-regs[i.c];break;
        case OR: regs[i.a]=regs[i.b]|regs[i.c];break;
        case M_AND: regs[i.a]=regs[i.b]&regs[i.c];break;
        case M_XOR: regs[i.a]=regs[i.b]^regs[i.c];break;
        case M_NOR: regs[i.a]=~(regs[i.b]|regs[i.c]);break;
        case SLL: regs[i.a]=regs[i.b]<<(regs[i.c]&63);break;
        case IMM: regs[i.a]=i.c;break;
        case LOAD: regs[i.a]=sx(read_mem(regs[i.b],i.size),i.size);break;
        case AMADD: case AMAND: case AMOR: case AMXOR:
            value=read_mem(regs[i.c],i.size);
            write_mem(regs[i.c],i.size,i.op==AMADD ? value+regs[i.b] :
                i.op==AMAND ? value&regs[i.b] : i.op==AMOR ? value|regs[i.b] : value^regs[i.b]);
            regs[i.a]=sx(value,i.size);break;
        case CAS:
            if(conflict && !injected) {
                actual_old=(old+1)&mask(size); write_mem(regs[i.c],size,actual_old); injected=1;
            }
            value=read_mem(regs[i.c],i.size);
            if((regs[i.a]&mask(i.size))==value) write_mem(regs[i.c],i.size,regs[i.b]);
            regs[i.a]=sx(value,i.size);break;
        case BNE:
            if(regs[i.a]!=regs[i.b]) {
                int label=0;
                while(label<count && !(code[label].op==LABEL && code[label].a==i.c)) label++;
                CHECK(label<count);pc=label;
            }
            break;
        case FLAGS: {
            if(op>=dt_X86_INS_BTS) {
                CHECK((regs[i.a]&mask(size))==actual_old);
                CHECK(i.a==i.b && regs[i.c]==src);
                flags_seen++;break;
            }
            uint64_t result = op==dt_X86_INS_NEG ? -actual_old :
                op==dt_X86_INS_ADC ? actual_old+src+cf : actual_old-src-cf;
            CHECK((regs[i.a]&mask(size))==(result&mask(size)));
            CHECK((regs[i.b]&mask(size))==(op==dt_X86_INS_NEG ? 0 : actual_old));
            CHECK((regs[i.c]&mask(size))==(op==dt_X86_INS_NEG ? actual_old : src));
            flags_seen++; carry=0; break;
        }
        case PAGE: CHECK(i.size==size/8 && regs[i.a]==regs[1]);break;
        case LABEL:break;
        default:CHECK(0);
        }
    }
    uint64_t result=op==dt_X86_INS_NEG ? -actual_old :
        op==dt_X86_INS_ADC ? actual_old+src+cf : actual_old-src-cf;
    if(op>=dt_X86_INS_BTS) {
        uint64_t bit=UINT64_C(1)<<src;
        result=op==dt_X86_INS_BTS ? actual_old|bit : op==dt_X86_INS_BTR ? actual_old&~bit : actual_old^bit;
    }
    CHECK(read_mem(regs[1],size)==(result&mask(size)) && flags_seen==1);
    for(int j=0;j<(int)sizeof(memory);j++)
        if(j<64+offset || j>=64+offset+size/8) CHECK(memory[j]==before[j]);
    CHECK(!clash || injected);
}
int main(int argc,char **argv) {
    if(argc>=2) {
        handle_arg_latx_unaligned_atomic(argv[1]);
        if(argc==3) capability=atoi(argv[2]);
        CHECK(latx_options_finalize());
        return option_unaligned_atomic;
    }
    unsigned long cases=0;
    for(int size=16;size<=64;size*=2) for(int op=4;op<=6;op++) {
        count=0;next_reg=3;next_label=0;live_temps=0;
        IR1_INST i={.opnd={{size},{size}},.opcode=op};
        CHECK(translate_btx_fast_atomic(&i));
        for(int off=0;off<64;off++) for(int bit=0;bit<size;bit++) {
            run(size,op,mask(size),bit,0,off,0);cases++;
            run(size,op,0,bit,1,off,0);cases++;
            if(size==16) {run(size,op,mask(size),bit,0,off,1);cases++;}
        }
    }
    for(int size=8;size<=64;size*=2) for(int op=1;op<=3;op++) {
        count=0;next_reg=3;next_label=0;live_temps=0;
        IR1_INST i={.opnd={{size},{size}},.opcode=op};
        CHECK(op==3 ? translate_lock_neg_fast_atomic(&i) : translate_lock_carry_fast_atomic(&i));
        uint64_t values[]={0,1,15,(UINT64_C(1)<<(size-1))-1,UINT64_C(1)<<(size-1),mask(size)-1,mask(size)};
        for(int off=0;off<64;off++) for(int a=0;a<7;a++) for(int b=0;b<7;b++) for(int cf=0;cf<2;cf++) {
            run(size,op,values[a],values[b],cf,off,0);cases++;
            if(op==3) {run(size,op,values[a],values[b],cf,off,1);cases++;}
        }
    }
    printf("PASS: %lu emitted-IR cases, neighbor bytes, CAS retry, carry input, option parser\n",cases);
    return 0;
}
'''

source = PRELUDE + '\n' + '\n'.join([
    function('linux-user/main.c', 'static void handle_arg_latx_unaligned_atomic('),
    function('target/i386/latx/latx-options.c', 'bool latx_options_finalize('),
    function('target/i386/latx/translator/tr-lock-fast-atomic.c', 'bool translate_lock_carry_fast_atomic('),
    function('target/i386/latx/translator/tr-lock-fast-atomic.c', 'bool translate_lock_neg_fast_atomic('),
    function('target/i386/latx/translator/tr-btx.c', 'static bool calculate_btx('),
    function('target/i386/latx/translator/tr-btx.c', 'static bool translate_btx_fast_atomic('),
]) + MAIN
with tempfile.TemporaryDirectory(prefix='latx-unaligned-atomic-') as d:
    path = Path(d)
    (path / 'test.c').write_text(source)
    cc = shlex.split(os.environ.get('CC', 'cc'))
    subprocess.run(cc + ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    for value, expected in [('0', 0), ('1', 1), ('2', 1), ('-1', 1), ('', 1), ('1x', 1)]:
        r = subprocess.run([str(path / 'test'), value], capture_output=True, text=True)
        if r.returncode != expected or (value not in ('0', '1') and 'expects 0 or 1' not in r.stderr):
            raise RuntimeError(f'parser {value!r}: {r.returncode} {r.stderr}')
    r = subprocess.run([str(path / 'test'), '1', '0'], capture_output=True, text=True)
    if r.returncode != 1 or 'requires LAM, LAM_BH and LAMCAS' not in r.stderr:
        raise RuntimeError('missing AM capability did not reject opt-in: ' + r.stderr)
    subprocess.run([str(path / 'test'), '0', '0'], check=True)
