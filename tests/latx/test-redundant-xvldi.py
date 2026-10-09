#!/usr/bin/env python3
"""Exercise the actual XVLDI pass, opcode classifiers and node removal."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"(?:static )?(?:void|bool|int) " + name +
                      r"\([^)]*\)\s*\{", source)
    if not match:
        raise RuntimeError("missing function: " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


HARNESS = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
typedef enum {
#include "ir2-opcode.h"
} IR2_OPCODE;
enum { IR2_OPND_NONE, IR2_OPND_FPR, IR2_OPND_GPR };
typedef struct { int _type, _reg_num, _imm32; } IR2_OPND;
typedef struct IR2_INST {
    IR2_OPCODE opcode;
    int id, _next, _prev, op_count;
    IR2_OPND _opnd[4];
} IR2_INST;
typedef struct {
    IR2_INST *first_ir2, *last_ir2, *ir2_inst_array;
} TRANSLATION_DATA;
static IR2_INST nodes[16];
static TRANSLATION_DATA data = { .ir2_inst_array = nodes };
static struct { TRANSLATION_DATA *tr_data; } env = { &data }, *lsenv = &env;
static IR2_INST *ir2_next(IR2_INST *p)
{ return p->_next < 0 ? NULL : nodes + p->_next; }
static IR2_INST *ir2_prev(IR2_INST *p)
{ return p->_prev < 0 ? NULL : nodes + p->_prev; }
static int ir2_get_id(IR2_INST *p) { return p->id; }
static IR2_OPCODE ir2_opcode(IR2_INST *p) { return p->opcode; }
static void ir2_set_opcode(IR2_INST *p, IR2_OPCODE o) { p->opcode = o; }
static void sequence(int count)
{
    memset(nodes, 0, sizeof(nodes));
    data.first_ir2 = nodes;
    data.last_ir2 = nodes + count - 1;
    for (int i = 0; i < count; ++i) {
        nodes[i].id = i;
        nodes[i]._prev = i - 1;
        nodes[i]._next = i + 1 == count ? -1 : i + 1;
        nodes[i].opcode = LISA_XVLDI;
        nodes[i].op_count = 2;
        nodes[i]._opnd[0] = (IR2_OPND){ IR2_OPND_FPR, 20, 0 };
        nodes[i]._opnd[1]._imm32 = 42;
    }
}
"""

CHECK = r"""
int main(void)
{
    sequence(3);
    nodes[1].opcode = LISA_X86_INST;
    nodes[1].op_count = 0;
    ir2_opt_redundant_xvldi();
    assert(nodes[2].opcode == LISA_INVALID);
    assert(data.last_ir2 == nodes + 1 && nodes[1]._next == -1);

    sequence(3);
    ir2_opt_redundant_xvldi();
    assert(nodes[1].opcode == LISA_INVALID && nodes[2].opcode == LISA_INVALID);
    assert(data.first_ir2 == nodes && data.last_ir2 == nodes);
    assert(nodes[0]._next == -1);

    sequence(2);
    nodes[1]._opnd[1]._imm32 = 43;
    ir2_opt_redundant_xvldi();
    assert(nodes[1].opcode == LISA_XVLDI);
    sequence(2);
    nodes[1]._opnd[0]._reg_num = 21;
    ir2_opt_redundant_xvldi();
    assert(nodes[1].opcode == LISA_XVLDI);

    const IR2_OPCODE barriers[] = {
        LISA_FADD_S, LISA_MOVGR2FR_D, LISA_VLDI, LISA_VLD, LISA_XVLD,
        LISA_XVADD_W, LISA_VEXTRINS_W, LISA_BEQ, LISA_BL, LISA_JIRL,
        LISA_LABEL
    };
    for (unsigned i = 0; i < ARRAY_SIZE(barriers); ++i) {
        sequence(3);
        nodes[1].opcode = barriers[i];
        ir2_opt_redundant_xvldi();
        assert(nodes[2].opcode == LISA_XVLDI);
    }
    const IR2_OPCODE stores[] = { LISA_FST_S, LISA_VST, LISA_XVST };
    for (unsigned i = 0; i < ARRAY_SIZE(stores); ++i) {
        sequence(3);
        nodes[1].opcode = stores[i];
        ir2_opt_redundant_xvldi();
        assert(nodes[2].opcode == LISA_INVALID);
    }
    /* Separate invocations must not carry constants across TBs. */
    sequence(1);
    ir2_opt_redundant_xvldi();
    sequence(1);
    ir2_opt_redundant_xvldi();
    assert(nodes[0].opcode == LISA_XVLDI);
    puts("PASS: redundant XVLDI elimination and invalidation");
    return 0;
}
"""


def main():
    ir2 = (ROOT / "target/i386/latx/ir2/ir2.c").read_text()
    optimization = (ROOT / "target/i386/latx/optimization/ir2-optimization.c").read_text()
    helpers = "\n".join(function(ir2, name) for name in (
        "ir2_opnd_is_freg", "ir2_opcode_is_branch", "ir2_opcode_is_jirl",
        "la_ir2_opcode_is_label", "la_ir2_opcode_is_store", "ir2_remove"))
    code = HARNESS + helpers + function(optimization, "ir2_opt_redundant_xvldi") + CHECK
    with tempfile.TemporaryDirectory(prefix="latx-xvldi-test-") as directory:
        path = Path(directory)
        (path / "test.c").write_text(code)
        subprocess.run(shlex.split(os.environ.get("CC", "cc")) + [
            "-std=c11", "-O2", "-Wall", "-Wextra", "-I",
            str(ROOT / "target/i386/latx/include"), str(path / "test.c"),
            "-o", str(path / "test")], check=True)
        return subprocess.run([str(path / "test")]).returncode


if __name__ == "__main__":
    raise SystemExit(main())
