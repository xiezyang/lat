#!/usr/bin/env python3
"""Check experimental floating-point encodings and 64-bit option selection."""
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
LATX = ROOT / "target/i386/latx"


def function(source, name):
    match = re.search(r"(?:static )?\w+ " + name + r"\([^)]*\)\s*\{", source)
    if not match:
        raise AssertionError("missing function: " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


class FloatOptionsTest(unittest.TestCase):
    def test_instruction_templates(self):
        templates = json.loads((LATX / "inst_template.json").read_text())
        expected = {
            "x86fadd.ld": 0x716d0000,
            "x86fsub.ld": 0x716d8000,
            "x86fmul.ld": 0x716e0000,
            "x86fdiv.ld": 0x716e8000,
            "x86fcom.ld": 0x71740000,
            "x86fucom.ld": 0x71748000,
            "x86fadd.ss": 0x72ac4000,
            "x86fadd.sd": 0x72ac4400,
            "x86fsub.ss": 0x72ac4800,
            "x86fsub.sd": 0x72ac4c00,
            "x86fmul.ss": 0x72ac5000,
            "x86fmul.sd": 0x72ac5400,
        }
        for name, opcode in expected.items():
            with self.subTest(name=name):
                template = templates[name]
                self.assertEqual(int(template["opcode"], 16), opcode)
                self.assertEqual(template["opnd"][:2], ["fpr_vd", "fpr_vj"])
                self.assertEqual(len(template["opnd"]), 2 if name[-2:] in
                                 ("ss", "sd") else 3)

    def test_stack_codegen_guards(self):
        spec = importlib.util.spec_from_file_location("convert", LATX / "convert.py")
        convert = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(convert)
        self.assertIn("option_softfpu && !option_stack",
                      convert.generatingFunctionBody("movgr2fcsr", "0", []))
        headers = []
        self.assertIn("option_softfpu && !option_stack",
                      convert.convertToConstructor("x86inctop", "0", [], headers))

    def test_high_option_bits(self):
        source = (LATX / "translator/translate.c").read_text()
        mask = function(source, "softfpu_fast_mask")
        planner = function(source, "softfpu_region_class")
        names = sorted(set(re.findall(r"dt_X86_INS_\w+", mask + planner)))
        header = (LATX / "include/latx-options.h").read_text()
        defines = "\n".join(re.findall(r"^#define LATX_SOFTFPU_FAST_.*$",
                                     header, re.M))
        handler = function((ROOT / "linux-user/main.c").read_text(),
                           "handle_arg_latx_softfpu_fast")
        harness = """
#include <stdint.h>
#include <stdlib.h>
typedef int IR1_OPCODE;
typedef enum { SOFTFPU_REGION_NONE, SOFTFPU_REGION_REQUIRED,
               SOFTFPU_REGION_TRANSPARENT } SoftFPURegionClass;
static uint64_t option_softfpu_fast;
static int option_float80;
""" + "enum {" + ",".join(names) + "};\n" + defines + "\n" + mask + planner + handler + """
int main(void) {
    handle_arg_latx_softfpu_fast("0x100000000");
    if (option_softfpu_fast != (UINT64_C(1) << 32)) return 1;
    option_float80 = 1;
    if (softfpu_region_class(dt_X86_INS_FUCOMP) !=
        SOFTFPU_REGION_TRANSPARENT) return 2;
    option_float80 = 0;
    if (softfpu_region_class(dt_X86_INS_FUCOMP) !=
        SOFTFPU_REGION_REQUIRED) return 3;
    handle_arg_latx_softfpu_fast("-1");
    if (option_softfpu_fast != UINT64_MAX) return 4;
    if (softfpu_region_class(dt_X86_INS_FCOMI) !=
        SOFTFPU_REGION_REQUIRED) return 5;
    if (softfpu_region_class(dt_X86_INS_FADDP) !=
        SOFTFPU_REGION_TRANSPARENT) return 6;
    option_float80 = 1;
    if (softfpu_region_class(dt_X86_INS_FCOMI) !=
        SOFTFPU_REGION_TRANSPARENT) return 7;
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as tmp:
            c_file = Path(tmp) / "options.c"
            binary = Path(tmp) / "options"
            c_file.write_text(harness)
            cc = shlex.split(os.environ.get("CC", "cc"))
            subprocess.run(cc + ["-std=c99", "-Wall", "-Wextra", "-Werror",
                                 str(c_file), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
