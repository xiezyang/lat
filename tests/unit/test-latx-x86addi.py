#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Check real immediate ADD flag selection and backend machine encoding."""
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
LATX = ROOT / 'target/i386/latx'


def function(path, name):
    source = re.sub(r"/\*.*?\*/|//[^\n]*", "", path.read_text(), flags=re.S)
    match = re.search(r'(?:static (?:inline )?)?\w+ \*?' + name +
                      r'\([^)]*\)\s*\{', source)
    if not match:
        raise AssertionError('missing function: ' + name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


def compile_run(source, includes=(), args=()):
    with tempfile.TemporaryDirectory() as work:
        binary = Path(work) / 'test'
        command = shlex.split(os.environ.get('CC', 'cc'))
        subprocess.run(command + ['-std=gnu11', '-Wall', '-Werror',
                                  '-Wno-unused-function', '-Wno-unused-variable',
                                  '-Wno-unused-but-set-variable'] +
                       ['-I' + str(path) for path in includes] +
                       ['-x', 'c', '-', '-o', str(binary)],
                       input=source, text=True, check=True)
        return subprocess.run([str(binary), *args], capture_output=True, text=True)


class X86AddiTest(unittest.TestCase):
    def test_template(self):
        templates = json.loads((LATX / 'inst_template.json').read_text())
        for suffix, field in [('w', 4), ('d', 5)]:
            template = templates['x86addi.' + suffix]
            self.assertEqual(int(template['opcode'], 0),
                             (5 << 20) | (field << 15) | (3 << 13))
            self.assertEqual(template['opnd'], ['gpr_rd', 'imm_si8l'])

    def test_existing_flag_selection(self):
        selector = function(LATX / 'optimization/flag-lbt.c',
                            'generate_eflag_by_lbt')
        wrapper = function(LATX / 'translator/tr-eflag-process.c',
                           'generate_eflag_calculation')
        opcodes = sorted(set(re.findall(r'dt_X86_INS_\w+', selector + wrapper)))
        macros = (LATX / 'include/flag-lbt.h').read_text()
        macros = re.sub(r'^#include.*$', '', macros, flags=re.M)
        emitters = []
        for operation in ['add', 'adc', 'inc', 'dec', 'sub', 'sbc', 'xor',
                          'and', 'or', 'sll', 'srl', 'sra', 'rcl', 'rcr',
                          'mul', 'slli', 'srli', 'srai']:
            for width, bits in [('b', 8), ('h', 16), ('w', 32), ('d', 64),
                                ('bu', 8), ('hu', 16), ('wu', 32), ('du', 64)]:
                emitters.append('#define la_x86%s_%s(...) record(%d, 0)\n' %
                                (operation, width, bits))
        source = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint32_t uint32;
typedef struct { int _type, _reg_num; } IR2_OPND;
typedef struct { int kind, size; int64_t value; } IR1_OPND;
typedef struct { int mnemonic; } INFO;
typedef struct { int opcode, live; IR1_OPND operands[2]; INFO *info; } IR1_INST;
static int calls, selected_bits, immediate_calls, selected_imm;
static int loads, allocations, frees;
static int option_x86addi;
static IR2_OPND zero_ir2_opnd;
static int record(int bits, int immediate) {
    calls++; selected_bits = bits; immediate_calls += immediate; return 0;
}
static void la_x86addi_w(IR2_OPND input, int value) {
    record(32, 1); selected_imm = value;
}
static void la_x86addi_d(IR2_OPND input, int value) {
    record(64, 1); selected_imm = value;
}
#define ir1_opcode(p) ((p)->opcode)
#define ir1_get_opnd(p, n) (&(p)->operands[n])
#define ir1_opnd_size(p) ((p)->size)
#define ir1_opnd_is_imm(p) ((p)->kind == 1)
#define ir1_opnd_simm(p) ((p)->value)
#define ir2_opnd_is_imm(p) ((p)->_type == 1)
#define ir2_opnd_imm(p) ((p)->_reg_num)
#define ir1_need_calculate_any_flag(p) ((p)->live)
#define SIGN_EXTENSION 0
#define lsassert(...) ((void)0)
#define lsassertm(...) ((void)0)
#define ir1_name(...) "mock"
static IR2_OPND ra_alloc_itemp(void) {
    allocations++; return (IR2_OPND){0, 20};
}
#define load_ireg_from_ir1_2(...) (++loads)
#define ra_free_temp(...) (++frees)
#define generate_af(...) ((void)0)
#define generate_cf(...) ((void)0)
#define generate_pf(...) ((void)0)
#define generate_zf(...) ((void)0)
#define generate_sf(...) ((void)0)
#define generate_of(...) ((void)0)
#define generate_cf_not_sx(...) ((void)0)
#define generate_of_not_sx(...) ((void)0)
#define ir1_need_calculate_pf(...) 0
#define ir1_need_calculate_af(...) 0
#define ir1_need_calculate_zf(...) 0
#define ir1_need_calculate_sf(...) 0
#define ir1_need_calculate_cf(...) 0
#define ir1_need_calculate_of(...) 0
'''
        source += 'enum {' + ','.join(opcodes) + '};\n'
        source += ''.join(emitters) + macros + '\n' + selector + '\n' + wrapper
        source += r'''
int main(void) {
    const int64_t values[] = {-2147483648LL, -129, -128, -1, 0, 1, 127, 128,
                              255, 2147483647LL, 4294967295LL};
    IR2_OPND src = {0, 9}, imm = {1, 1};
    for (option_x86addi = 0; option_x86addi <= 1; option_x86addi++)
    for (int bits = 8; bits <= 64; bits *= 2)
    for (int kind = 0; kind <= 1; kind++)
    for (int live = 0; live <= 1; live++)
    for (unsigned i = 0; i < sizeof(values)/sizeof(values[0]); i++) {
        IR1_INST inst = {.opcode = dt_X86_INS_ADD, .live = live,
            .operands = {{0, bits, 0}, {kind, 32, values[i]}}};
        calls = immediate_calls = loads = allocations = frees = 0;
        generate_eflag_calculation(src, src, imm, &inst, true);
        int expected = option_x86addi && live && kind && bits >= 32 &&
                       values[i] >= -128 && values[i] <= 127;
        if (calls != live || immediate_calls != expected ||
            (live && selected_bits != bits) ||
            (expected && (selected_imm != values[i] || loads || allocations))) {
            fprintf(stderr, "bits=%d kind=%d live=%d imm=%lld calls=%d new=%d\n",
                    bits, kind, live, (long long)values[i], calls, immediate_calls);
            return 1;
        }
    }
    option_x86addi = 1;
    /* ADD without immediate permission and XADD retain register flag ops. */
    IR1_INST inst = {.opcode = dt_X86_INS_ADD, .live = 1,
                    .operands = {{0, 64, 0}, {1, 8, -1}}};
    calls = immediate_calls = 0;
    generate_eflag_by_lbt(src, src, imm, &inst, false);
    if (calls != 1 || immediate_calls) return 2;
    inst.opcode = dt_X86_INS_XADD;
    calls = immediate_calls = 0;
    generate_eflag_by_lbt(src, src, imm, &inst, true);
    if (calls != 1 || immediate_calls) return 3;
    return 0;
}
'''
        result = compile_run(source)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_immediate_result_selection(self):
        selector = function(LATX / 'translator/translate.c', 'tr_opt_simm12')
        source = r"""
#include <stdbool.h>
#include <stdint.h>
typedef struct { int size; int64_t value; } IR1_OPND;
typedef struct { int opcode, live, simm12; IR1_OPND operands[2]; } IR1_INST;
enum { dt_X86_INS_ADD, dt_X86_INS_ADC, dt_X86_INS_SUB };
#define ir1_opcode(p) ((p)->opcode)
#define ir1_get_opnd(p,n) (&(p)->operands[n])
#define ir1_opnd_size(p) ((p)->size)
#define ir1_opnd_simm(p) ((p)->value)
#define ir1_opnd_is_simm12(p) (inst.simm12)
#define ir1_need_calculate_any_flag(p) ((p)->live)
static IR1_INST inst;
static int option_x86addi;
"""
        source += selector + r"""
int main(void) {
 const int64_t values[] = {-2048,-129,-128,-1,0,127,128,2047,2048};
 for(option_x86addi=0;option_x86addi<=1;option_x86addi++)
 for(int op=0;op<3;op++) for(int bits=8;bits<=64;bits*=2)
 for(int live=0;live<2;live++) for(int simm12=0;simm12<2;simm12++)
 for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++) {
  inst=(IR1_INST){.opcode=op,.live=live,.simm12=simm12,
       .operands={{bits,0},{32,values[i]}}};
#ifdef CONFIG_LATX_FLAG_REDUCTION
  bool expected=simm12 && (!live || (option_x86addi && op==dt_X86_INS_ADD && bits>=32 &&
                                     values[i]>=-128 && values[i]<=127));
#else
  bool expected=false;
#endif
  if(tr_opt_simm12(&inst)!=expected) return 1;
 }
 return 0;
}
"""
        for config in ('', '#define CONFIG_LATX_FLAG_REDUCTION 1\n'):
            result = compile_run(config + source)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_option_and_aot_independence(self):
        self.assertRegex(function(LATX / 'latx-options.c', 'options_init'),
                         r'option_x86addi\s*=\s*0\s*;')
        main = (ROOT / 'linux-user/main.c').read_text()
        self.assertRegex(main, r'\{"latx-x86addi",\s*"LATX_X86ADDI",\s*true,\s*'
                              r'handle_arg_latx_x86addi,')
        self.assertIn('ENVFUN(LATX_X86ADDI, handle_arg_latx_x86addi)',
                      (LATX / 'include/latx-options.h').read_text())
        source = r"""
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static int option_x86addi, option_aot;
"""
        source += function(ROOT / 'linux-user/main.c', 'handle_arg_latx_x86addi')
        source += function(LATX / 'latx-options.c', 'latx_options_finalize')
        source += r"""
int main(int argc, char **argv) {
 for(int aot=0;aot<=1;aot++) {
  option_aot=aot;
  handle_arg_latx_x86addi(argv[1]);
  if(!latx_options_finalize() || option_aot!=aot ||
     option_x86addi!=(argv[1][0]=='1')) return 1;
 }
 return 0;
}
"""
        for value in ('0', '1'):
            result = compile_run(source, args=(value,))
            self.assertEqual(result.returncode, 0, result.stderr)
        for value in ('', '2', '-1', '0x1', '1junk'):
            result = compile_run(source, args=(value,))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('must be 0 or 1', result.stderr)

    def test_real_backend_encoding(self):
        with tempfile.TemporaryDirectory() as work:
            work = Path(work)
            (work / 'include').mkdir()
            (work / 'ir2').mkdir()
            (work / 'inst_template.json').write_text(
                (LATX / 'inst_template.json').read_text())
            subprocess.run(['python3', str(LATX / 'convert.py')],
                           cwd=work, check=True, capture_output=True)
            assembler = (LATX / 'ir2/ir2-assemble.c').read_text()
            assembler = re.sub(r'^#include "(?!format-table)[^"]*"\n', '',
                               assembler, flags=re.M)
            types = (LATX / 'include/ir2-la-assemble.h').read_text()
            types = types[types.index('typedef enum'):types.index('typedef struct {\n    GM_OPERAND_TYPE place;')]
            source = r'''
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef uint32_t uint32;
typedef enum {
#include "ir2-opcode.h"
} IR2_OPCODE;
typedef struct { int _val; } IR2_OPND;
typedef struct { int _opcode, op_count; IR2_OPND _opnd[4]; } IR2_INST;
#define lsassert assert
#define lsassertm(a, ...) assert(a)
#define ir2_opcode(p) ((p)->_opcode)
#define ir2_opnd_is_pseudo(p) false
#define ir2_opnd_is_data(p) false
#define ir2_opnd_is_ireg(p) true
static IR2_INST storage;
static IR2_INST *ir2_allocate(void) { return &storage; }
static IR2_OPND create_immh_opnd(int value) { return (IR2_OPND){value}; }
static int64_t sextract64(uint64_t value, int start, int width) {
    return (int64_t)(value << (64 - start - width)) >> (64 - width);
}
'''
            source += types + '\n' + assembler + '\n'
            disassembler = (LATX / 'ir2/la-disasm.c').read_text()
            source += re.search(r'static bool is_la_sign_opnd\[\] = \{.*?\};',
                                disassembler, re.S).group() + '\n'
            source += function(LATX / 'ir2/la-disasm.c', 'la_get_opnd_val') + '\n'
            source += function(LATX / 'ir2/la-decode.c', 'la_decode') + '\n'
            # la_decode uses compatibility macros for unsupported opcodes.
            decoder = (LATX / 'ir2/la-decode.c').read_text()
            source = '\n'.join(line for line in decoder.splitlines()
                               if line.startswith('#define ')) + '\n' + source
            for suffix in ('w', 'd'):
                source += function(work / 'ir2/la-append.c',
                                   'generate_x86addi_' + suffix) + '\n'
            source += r'''
int main(void) {
    for (int bits = 32; bits <= 64; bits += 32)
    for (int imm = -128; imm <= 127; imm++)
    for (int reg = 0; reg < 32; reg++) {
        IR2_OPND input = {reg};
        IR2_INST *inst = bits == 32 ? generate_x86addi_w(input, imm) :
                                     generate_x86addi_d(input, imm);
        uint32_t word = ir2_assemble(inst);
        uint32_t expected = (5U << 20) | ((bits == 32 ? 4U : 5U) << 15) |
                            (3U << 13) | ((unsigned)(imm & 255) << 5) | reg;
        if (word != expected || la_decode(word) != inst->_opcode ||
            la_get_opnd_val(IMM_SI8L, word) != imm ||
            la_get_opnd_val(GPR_RD, word) != reg) return 1;
    }
    return 0;
}
'''
            result = compile_run(source, [work / 'include'])
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
