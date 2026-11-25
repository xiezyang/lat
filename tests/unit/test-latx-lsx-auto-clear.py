#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise the real YMM clear emitters and pending state with a vector model."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function(path, name):
    source = (ROOT / path).read_text()
    match = re.search(r"(?:static )?\w+ " + name + r"\([^)]*\)\s*\{", source)
    if not match:
        raise AssertionError("missing function: " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


class LsxAutoClearTest(unittest.TestCase):
    def compile_run(self, source, args=()):
        with tempfile.TemporaryDirectory() as work:
            binary = Path(work) / "test"
            cc = shlex.split(os.environ.get("CC", "cc"))
            subprocess.run(cc + ["-std=c99", "-Wall", "-Wextra", "-Werror",
                                 "-x", "c", "-", "-o", str(binary)],
                           input=source, text=True, check=True)
            return subprocess.run([str(binary), *args], capture_output=True)

    def test_option(self):
        source = """
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int option_lsx_auto_clear;
""" + function("linux-user/main.c", "handle_arg_latx_lsx_auto_clear") + """
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    handle_arg_latx_lsx_auto_clear(argv[1]);
    return option_lsx_auto_clear != (argv[1][0] == '1');
}
"""
        for value in ("0", "1"):
            result = self.compile_run(source, (value,))
            self.assertEqual(result.returncode, 0, result.stderr)
        for value in ("", "2", "-1", "0x1", "1junk"):
            result = self.compile_run(source, (value,))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(b"must be 0 or 1", result.stderr)

    def test_clear_emitters(self):
        source = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
enum { LISA_INVALID, LISA_X86_INST, LISA_LABEL, LISA_PSEUDO_END,
       VADD, XVADD, VSTORE, FADD, JUMP,
       LISA_X86FADD_LD, LISA_X86FUCOM_LD,
       LISA_X86FADD_SS, LISA_X86FMUL_SD };
enum { IR2_OPND_NONE, IR2_OPND_FPR, FPR_VD, FPR_XD };
typedef int IR2_OPCODE;
typedef struct { int _type, _reg_num; } IR2_OPND;
typedef struct IR2_INST {
    IR2_OPCODE opcode;
    int op_count;
    IR2_OPND _opnd[1];
    struct IR2_INST *prev;
} IR2_INST;
typedef struct { int opnd[1]; } GM_LA_OPCODE_FORMAT;
static GM_LA_OPCODE_FORMAT lisa_format_table[32];
"""
        source += r"""
typedef struct { uint16_t ymmh_zero_pending; IR2_INST *last_ir2; } Data;
static Data tr_data;
static struct { Data *tr_data; } env = { &tr_data }, *lsenv = &env;
static int option_enable_lasx = 1, option_lsx_auto_clear;
static const int reg_xmm_map[16] = {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
static uint64_t regs[32][4];
static IR2_INST nodes[128];
static int node_count, clears, copies, allocations;
#define XMM0_MAPS 8
#define XMM15_MAPS 23
#define CPU_NB_REGS 16
#define lsassert assert
#define XVPERMI_Q_4_0(n,m) (((n) << 4) | (m))
static IR2_OPND reg(int n) { return (IR2_OPND){IR2_OPND_FPR, n}; }
static int ir2_opnd_base_reg_num(const IR2_OPND *r) { return r->_reg_num; }
static int ir2_opnd_is_freg(const IR2_OPND *r) { return r->_type == IR2_OPND_FPR; }
static int ir2_opnd_cmp(const IR2_OPND *a, const IR2_OPND *b) {
    return a->_type == b->_type && a->_reg_num == b->_reg_num;
}
static IR2_OPCODE ir2_opcode(const IR2_INST *i) { return i->opcode; }
static IR2_INST *ir2_prev(const IR2_INST *i) { return i->prev; }
static bool ir2_opcode_is_branch(int opcode) { return opcode == JUMP; }
static bool ir2_opcode_is_jirl(int opcode) { (void)opcode; return false; }
static bool la_ir2_opcode_is_store(int opcode) { return opcode == VSTORE; }
static IR2_OPND ir2_opnd_new_none(void) { return (IR2_OPND){0, 0}; }
static void ir2_opnd_build(IR2_OPND *r, int type, int n) { *r = (IR2_OPND){type, n}; }
static IR2_OPND ra_alloc_ftemp(void) { ++allocations; return reg(31); }
static void ra_free_temp(IR2_OPND r) { (void)r; --allocations; }
static void append(int opcode, int dest) {
    IR2_INST *n = &nodes[node_count++];
    n->opcode = opcode;
    n->op_count = 1;
    n->_opnd[0] = reg(dest);
    n->prev = tr_data.last_ir2;
    tr_data.last_ir2 = n;
}
static void la_vori_b(IR2_OPND d, IR2_OPND s, int imm) {
    assert(imm == 0);
    regs[d._reg_num][0] = regs[s._reg_num][0];
    regs[d._reg_num][1] = regs[s._reg_num][1];
    if (option_lsx_auto_clear) regs[d._reg_num][2] = regs[d._reg_num][3] = 0;
    ++copies;
    append(VADD, d._reg_num);
}
static void la_vxor_v(IR2_OPND d, IR2_OPND a, IR2_OPND b) {
    assert(a._reg_num == b._reg_num);
    regs[d._reg_num][0] = regs[d._reg_num][1] = 0;
    if (option_lsx_auto_clear) regs[d._reg_num][2] = regs[d._reg_num][3] = 0;
    append(VADD, d._reg_num);
}
static void la_xvpermi_q(IR2_OPND d, IR2_OPND s, int imm) {
    assert(imm == 2);
    regs[d._reg_num][2] = regs[s._reg_num][0];
    regs[d._reg_num][3] = regs[s._reg_num][1];
    ++clears;
    append(XVADD, d._reg_num);
}
"""
        names = ("mapped_ymm_index", "last_write_clears_ymmh", "emit_high128_clear",
                 "cancel_pending_ymmh_zero", "flush_pending_ymmh_zero",
                 "emit_ymmh_zeros", "set_high128_xreg_to_zero")
        for name in names:
            source += function("target/i386/latx/translator/tr-opnd-process.c", name) + "\n"
        source += r"""
static void reset(int enabled) {
    memset(&tr_data, 0, sizeof(tr_data));
    memset(regs, 0, sizeof(regs));
    option_lsx_auto_clear = enabled;
    option_enable_lasx = 1;
    node_count = clears = copies = allocations = 0;
    regs[8][0] = 11; regs[8][1] = 22;
    regs[8][2] = 33; regs[8][3] = 44;
}
int main(void) {
    lisa_format_table[VADD].opnd[0] = FPR_VD;
    lisa_format_table[VSTORE].opnd[0] = FPR_VD;
    lisa_format_table[XVADD].opnd[0] = FPR_XD;
    lisa_format_table[LISA_X86FADD_LD].opnd[0] = FPR_VD;
    for (int enabled = 0; enabled <= 1; ++enabled) {
        reset(enabled);
        la_vori_b(reg(8), reg(8), 0);
        append(VSTORE, 8); /* A read of the same register is not a writer. */
        tr_data.ymmh_zero_pending = 1; /* Older pending state must be cancelled. */
        set_high128_xreg_to_zero(reg(8));
        if (tr_data.ymmh_zero_pending != !enabled) return 1;
        emit_ymmh_zeros();
        if (clears != !enabled || copies != 1 || allocations) return 2;
        if (regs[8][0] != 11 || regs[8][1] != 22 || regs[8][2] || regs[8][3]) return 3;

        reset(enabled);
        append(XVADD, 8); /* LASX writes do not clear the high half. */
        set_high128_xreg_to_zero(reg(8));
        flush_pending_ymmh_zero(reg(8));
        if (tr_data.ymmh_zero_pending || clears != !enabled || copies != enabled) return 4;
        if (regs[8][0] != 11 || regs[8][1] != 22 || regs[8][2] || regs[8][3]) return 5;

        reset(enabled); /* VZEROUPPER has no preceding register write. */
        set_high128_xreg_to_zero(reg(8));
        emit_ymmh_zeros();
        if (regs[8][2] || regs[8][3] || clears != !enabled || copies != enabled) return 6;
    }
    for (int barrier = LISA_X86_INST; barrier <= LISA_LABEL; ++barrier) {
        reset(1);
        append(VADD, 8);
        append(barrier, 0);
        set_high128_xreg_to_zero(reg(8));
        if (tr_data.ymmh_zero_pending != 1) return 7;
    }
    reset(1);
    append(LISA_X86FADD_LD, 8);
    if (last_write_clears_ymmh(reg(8))) return 8;
    reset(1);
    option_enable_lasx = 0;
    set_high128_xreg_to_zero(reg(8));
    if (node_count || tr_data.ymmh_zero_pending) return 9;
    reset(1);
    la_vori_b(reg(30), reg(8), 0);
    set_high128_xreg_to_zero(reg(30));
    if (copies != 1 || clears || allocations) return 10;
    return 0;
}
"""
        result = self.compile_run(source)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
