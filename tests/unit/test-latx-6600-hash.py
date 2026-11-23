#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile the real HASH lookup, option and signal-unlink code with mocks."""
from pathlib import Path
import re
import json
import shlex
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function(path, name):
    source = (ROOT / path).read_text()
    matches = list(re.finditer(r"(?:static (?:inline )?)?\w+ " + name +
                               r"\([^)]*\)\s*\{", source))
    if not matches:
        raise AssertionError("missing function: " + name)
    match = matches[-1]
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


class Hash6600Test(unittest.TestCase):
    def compile_run(self, source, defines=(), args=()):
        with tempfile.TemporaryDirectory() as work:
            binary = Path(work) / "test"
            command = shlex.split(os.environ.get("CC", "cc"))
            subprocess.run(command + ["-std=c11", "-Wall", "-Werror"] +
                           ["-D" + value for value in defines] +
                           ["-x", "c", "-", "-o", str(binary)],
                           input=source, text=True, check=True)
            return subprocess.run([str(binary), *args], capture_output=True)

    def test_instruction_encoding(self):
        template = json.loads((ROOT / "target/i386/latx/inst_template.json").read_text())["hash"]
        self.assertEqual(int(template["opcode"], 0),
                         (0 << 26) | (3 << 20) | (12 << 15) | (0 << 11))
        self.assertEqual(template["opnd"], ["gpr_rd", "gpr_rj", "imm_ui1"])

    def test_cache_indices(self):
        lookup = function("include/exec/tb-hash.h", "tb_jmp_cache_hash_func")
        for bits in (32, 64):
            source = """
#include <stdint.h>
#define CONFIG_LATX 1
#define TB_JMP_CACHE_BITS 16
#define TB_JMP_CACHE_SIZE (1 << TB_JMP_CACHE_BITS)
static int option_hash;
""" + f"typedef uint{bits}_t target_ulong;\n" + lookup + """
static int check(target_ulong pc) {
    option_hash = 0;
    if (tb_jmp_cache_hash_func(pc) != ((pc ^ (pc >> 16)) & 65535))
        return 1;
    unsigned expected = 0;
    for (unsigned b = 0; b < 16; ++b)
        expected |= (((pc >> b) ^ (pc >> (b + 16))) & 1) << b;
    option_hash = 1;
    return tb_jmp_cache_hash_func(pc) != expected;
}
int main(void) {
    target_ulong pc = 0;
    if (check(pc) || check(~pc)) return 1;
    option_hash = 1;
    for (unsigned i = 0; i < 65536; ++i)
        if (tb_jmp_cache_hash_func(i) != i) return 5;
    for (unsigned b = 0; b < sizeof(pc) * 8; ++b)
        if (check((target_ulong)1 << b)) return 2;
    for (unsigned i = 0; i < 100000; ++i) {
        pc = pc * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
        if (check(pc)) return 3;
        if (tb_jmp_cache_hash_func(pc) != tb_jmp_cache_hash_func(pc ^ 0x10001))
            return 4;
    }
    return 0;
}
"""
            with self.subTest(bits=bits):
                result = self.compile_run(source)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_lookup_and_signal_patch_offsets(self):
        source = """
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
typedef int IR2_OPND;
typedef int IR2_INST;
typedef uintptr_t ADDR;
struct tb_tc { void *ptr; };
typedef struct { uint64_t pc, cflags; struct tb_tc tc; uintptr_t jmp_indirect; }
    TranslationBlock;
typedef struct { int tcg_cflags; } CPUState;
typedef struct { uintptr_t insn_save[2]; CPUState cpu; } CPUArchState;
typedef int ucontext_t;
#define CONFIG_LATX 1
#define CF_PARALLEL 1
#define TB_JMP_CACHE_BITS 16
#define V0_RENAME_OPND 10
#define B_EPILOGUE_RET_0 0
#define qatomic_read(p) (*(p))
#define qatomic_set(p,v) (*(p)=(v))
static int option_hash, close_latx_parallel;
static uintptr_t tcg_splitwx_diff;
static const int zero_ir2_opnd;
static uint32_t code[64];
static int count, hashes, scale, shift;
static uint64_t regs[32], lookup_offset;
static int loads;
static void emit(void) { code[count++] = 0x02000000; }
#define la_srli_d(d,s,i) do { regs[d]=regs[s] >> (i); emit(); } while (0)
#define la_xor(d,s,t) do { regs[d]=regs[s]^regs[t]; emit(); } while (0)
#define la_bstrpick_d(d,s,h,l) do { regs[d]=(regs[s] >> (l)) & ((1ULL << ((h)-(l)+1))-1); emit(); } while (0)
#define la_pcaddi(...) emit()
#define la_ld_d(d,s,i) do { if (!loads++) lookup_offset=regs[s]+(i); emit(); } while (0)
#define la_masknez(...) emit()
#define la_maskeqz(...) emit()
#define la_or(...) emit()
#define la_bne(...) emit()
#define la_ldx_d(d,s,t) do { if (!loads++) lookup_offset=regs[s]+regs[t]; emit(); } while (0)
#define la_beq(...) emit()
#define la_bstrpick_w(...) emit()
#define la_alsl_d(d,s,b,i) do { scale=(i); regs[d]=(regs[s] << ((i)+1))+regs[b]; emit(); } while (0)
#define la_add_d(d,s,t) do { regs[d]=regs[s]+regs[t]; emit(); } while (0)
#define la_slli_d(d,s,i) do { shift=(i); regs[d]=regs[s] << (i); emit(); } while (0)
#define la_hash(d,s,m) do { assert((m)==1); regs[d]=((regs[s]^(regs[s] >> 16)) & 65535) << 4; ++hashes; emit(); } while (0)
#define la_jirl(...) do { code[count++] = 0x4c000000; } while (0)
#define la_data_li(...) ((void)0)
#define la_label(...) ((void)0)
#define ra_free_temp(...) ((void)0)
#define aot_la_append_ir2_jmp_far(...) ((void)0)
static IR2_OPND ra_alloc_dbt_arg2(void) { return 2; }
static IR2_OPND ra_alloc_data(void) { return 3; }
static IR2_OPND ra_alloc_label(void) { return 4; }
static IR2_OPND ra_alloc_itemp(void) { return 5; }
static IR2_OPND ra_alloc_static0(void) { return 6; }
static IR2_INST *generate_andi(int d, int s, int i) {
    static int instruction;
    return &instruction;
}
static uint32_t ir2_assemble(IR2_INST *inst) { return 0x03400000; }
static CPUState *env_cpu(CPUArchState *env) { return &env->cpu; }
static void flush_idcache_range(uintptr_t start, uintptr_t end, int size) {}
""" + function("target/i386/latx/translator/translate.c",
               "generate_indirect_goto") + "\n" + function(
                   "target/i386/latx/latx-signal.c", "unlink_indirect_jmp") + """
int main(void) {
    CPUArchState env = {0};
    TranslationBlock tb = {.tc = {.ptr = code}};
    ucontext_t uc = 0;
    uint64_t pc = 0;
    for (unsigned input = 0; input < 1000; ++input) {
        pc = pc * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
        for (int mode = 0; mode < 2; ++mode) {
            option_hash = mode;
            hashes = count = loads = 0;
            memset(regs, 0, sizeof(regs));
            regs[2] = pc;
            scale = shift = -1;
            memset(code, 0, sizeof(code));
            generate_indirect_goto(code);
            int hit = count - 1;
            if (hashes != mode || code[hit] != 0x4c000000) return 1;
#ifdef CONFIG_LATX_FAST_JMPCACHE
            if (scale != (mode ? -1 : 3)) return 2;
            if (lookup_offset != (((pc ^ (pc >> 16)) & 65535) << 4)) return 9;
#else
            if (shift != (mode ? -1 : 3)) return 3;
            if (lookup_offset != (((pc ^ (pc >> 16)) & 65535) << 3)) return 10;
#endif
            for (int parallel = 0; parallel < 2; ++parallel) {
                uint32_t saved[64];
                memcpy(saved, code, sizeof(code));
                env.cpu.tcg_cflags = parallel ? CF_PARALLEL : 0;
                unlink_indirect_jmp(&env, &tb, &uc);
                int patch = (env.insn_save[0] - (uintptr_t)code) / 4;
#ifdef CONFIG_LATX_FAST_JMPCACHE
                if (patch != hit) return 4;
#else
                if (mode && patch != hit) return 5;
                if (!mode && patch != (parallel ? 0 : 7)) return 6;
#endif
                if (code[patch] != 0x03400000 || env.insn_save[1] != saved[patch])
                    return 7;
                for (int n = 0; n < count; ++n)
                    if (n != patch && code[n] != saved[n]) return 8;
                memcpy(code, saved, sizeof(code));
            }
        }
    }
    return 0;
}
"""
        # Mock emitters intentionally ignore operands unused by this check.
        source = '#pragma GCC diagnostic ignored "-Wunused-variable"\n' + source
        source = '#pragma GCC diagnostic ignored "-Wunused-function"\n' + source
        for flags in ((), ("CONFIG_LATX_FAST_JMPCACHE",),
                      ("CONFIG_LATX_FAST_JMPCACHE", "CONFIG_LATX_GLUE_MASK")):
            with self.subTest(flags=flags):
                result = self.compile_run(source, flags)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_option_validation_and_aot(self):
        source = """
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int option_hash, option_aot;
""" + function("linux-user/main.c", "handle_arg_latx_hash") + "\n" + function(
            "target/i386/latx/latx-options.c", "latx_options_finalize") + """
int main(int argc, char **argv) {
    option_aot = 1;
    handle_arg_latx_hash(argv[1]);
    if (!latx_options_finalize()) return 2;
    return option_aot != 1;
}
"""
        for value in ("0", "1"):
            result = self.compile_run(source, args=(value,))
            self.assertEqual(result.returncode, 0, result.stderr)
        for value in ("", "2", "-1", "0x1", "1junk"):
            result = self.compile_run(source, args=(value,))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(b"must be 0 or 1", result.stderr)


if __name__ == "__main__":
    unittest.main()
