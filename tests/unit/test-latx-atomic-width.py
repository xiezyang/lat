#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Execute real atomic interpreter bodies with legacy markers and ordinary NOPs.

Page relocation and signal registers are modelled, not kernel signal delivery.
"""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
production = (ROOT / 'accel/tcg/translate-all.c').read_text()


def function(signature):
    start = production.index(signature)
    return production[start:production.index('\n}', start) + 2]


prelude = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define LOCKINT_OK 0
#define LOCKINT_BAD 1
#define LOCKINT_SEGV 2
#define qemu_log_mask(...) ((void)0)
#define UC_GR(c) ((c)->regs)
#define UC_PC(c) ((c)->pc)
typedef uintptr_t abi_ulong;
#define ucontext_t Context
typedef struct { uint64_t regs[32]; uintptr_t pc; } Context;
static uint64_t qemu_host_page_mask = ~(uint64_t)4095;
static unsigned char memory[4096] __attribute__((aligned(4096)));
static void *set_interpret_newpage(uintptr_t page, int n)
{
    CHECK(page == (uintptr_t)memory && n == 1);
    return (void *)page;
}
static int recover_interpret_oldpage(uintptr_t page, void *p, int n)
{
    CHECK(page == (uintptr_t)p && n == 1);
    return LOCKINT_OK;
}
static int lock_interpret_check(uint64_t address)
{
    CHECK(address >= (uintptr_t)memory && address < (uintptr_t)(memory + sizeof(memory)));
    return 0;
}
'''
main = r'''
int main(void)
{
    /* Only ANDI zero, zero, 16/32 describes a legacy narrow access.
     * In particular ANDI zero, zero, 0 is an ordinary NOP.
     */
    uint32_t prefixes[] = {
        0, 0x03400000, 0x03400400, 0x03404000, 0x03408000,
        0x03410000, 0x03404001, 0x00150000
    };
    unsigned cases = 0;
    for (int op = 0; op < 4; op++) {
        for (int size = op ? 32 : 8; size <= 64; size *= 2) {
            for (unsigned p = 0; p < sizeof(prefixes) / sizeof(prefixes[0]); p++) {
                int legacy = prefixes[p] == 0x03404000 || prefixes[p] == 0x03408000;
                int bits = legacy ? (int)((prefixes[p] >> 10) & 0xfff) : size;
                int bytes = bits / 8;
                uint64_t mask = bits == 64 ? UINT64_MAX : (UINT64_C(1) << bits) - 1;
                for (int offset = 0; offset < 3; offset++) {
                    for (int value = 0; value < 2; value++) {
                        unsigned char saved[sizeof(memory)];
                        uint64_t initial = value ? mask : 0, actual = 0;
                        unsigned char *address = memory + 128 + offset;
                        memset(memory, 0xa5, sizeof(memory));
                        memcpy(address, &initial, bytes);
                        memcpy(saved, memory, sizeof(memory));
                        uint32_t inst[] = {prefixes[p], prefixes[p], 4 | (5 << 5) | (6 << 10)};
                        ucontext_t uc = {.pc = (uintptr_t)&inst[2]};
                        uc.regs[5] = (uintptr_t)address;
                        uc.regs[6] = 0x39;
                        uint64_t fixed = 0;
                        int (*interpret)(ucontext_t *, uint32_t *, uint64_t *, int);
                        interpret = op == 0 ? interpret_add : op == 1 ? interpret_and :
                                    op == 2 ? interpret_or : interpret_xor;
                        int result = interpret(&uc, &inst[2], &fixed, size);
                        CHECK(result == LOCKINT_OK && fixed == (uintptr_t)address);
                        CHECK(uc.pc == (uintptr_t)&inst[2] + 4);
                        CHECK(uc.regs[4] == (value ? UINT64_MAX : 0));
                        memcpy(&actual, address, bytes);
                        uint64_t expected = op == 0 ? initial + 0x39 :
                                            op == 1 ? initial & 0x39 :
                                            op == 2 ? initial | 0x39 : initial ^ 0x39;
                        CHECK(actual == (expected & mask));
                        for (unsigned n = 0; n < sizeof(memory); n++) {
                            if (n < 128u + offset || n >= 128u + offset + bytes) {
                                CHECK(memory[n] == saved[n]);
                            }
                        }
                        cases++;
                    }
                }
            }
        }
    }
    printf("PASS: %u atomic interpreter width cases, NOPs, legacy markers and adjacent bytes\n", cases);
    return 0;
}
'''
helper = 'static bool is_atomic_width_marker('
source = prelude + (function(helper) if helper in production else '') + '\n'
source += function('static int interpret_add(') + '\n'
source += function('static int interpret_and(') + '\n'
source += function('static int interpret_or(') + '\n'
source += function('static int interpret_xor(') + '\n' + main
with tempfile.TemporaryDirectory(prefix='latx-atomic-width-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(source)
    for defines in [[], ['-DNDEBUG']]:
        subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', *defines,
            str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
