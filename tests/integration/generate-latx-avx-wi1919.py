#!/usr/bin/env python3
"""Generate the four independent WI-1919 native shuffle fixtures."""

import argparse
import json
from pathlib import Path
from textwrap import dedent


MNEMONICS = ("vpshufb", "vpshufd", "vpshufhw", "vpshuflw")

DATA = r'''
    .section .rodata
    .balign 32
data_a:
    .byte 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07
    .byte 0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    .byte 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17
    .byte 0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
data_b:
    .byte 0x80,0x01,0x02,0x83,0x04,0x05,0x86,0x07
    .byte 0x08,0x89,0x0a,0x0b,0x8c,0x0d,0x0e,0x0f
    .byte 0x10,0x91,0x12,0x13,0x14,0x95,0x16,0x17
    .byte 0x18,0x19,0x9a,0x1b,0x1c,0x1d,0x1e,0x9f
'''


def assembly(mnemonic: str) -> str:
    op = mnemonic
    lines = [
        "/* SPDX-License-Identifier: GPL-2.0-only */",
        "    .intel_syntax noprefix",
        "    .text",
        f"    .globl latx_avx_single_{op}_run",
        f"    .type latx_avx_single_{op}_run, @function",
        f"latx_avx_single_{op}_run:",
    ]
    if op == "vpshufb":
        cases = [
            "movdqu xmm0, xmmword ptr [rip + data_a]; movdqu xmm1, xmmword ptr [rip + data_b]; vpshufb xmm2, xmm0, xmm1; vmovdqu ymmword ptr [rdi+0], ymm2",
            "movdqu xmm0, xmmword ptr [rip + data_a]; movdqu xmm1, xmmword ptr [rip + data_b]; vpshufb xmm0, xmm0, xmm1; vmovdqu ymmword ptr [rdi+32], ymm0",
            "movdqu xmm0, xmmword ptr [rip + data_a]; movdqu xmm1, xmmword ptr [rip + data_b]; vpshufb xmm1, xmm0, xmm1; vmovdqu ymmword ptr [rdi+64], ymm1",
            "movdqu xmm0, xmmword ptr [rip + data_a]; vpshufb xmm2, xmm0, xmmword ptr [rip + data_b]; vmovdqu ymmword ptr [rdi+96], ymm2",
            "vmovdqu ymm0, ymmword ptr [rip + data_a]; vmovdqu ymm1, ymmword ptr [rip + data_b]; vpshufb ymm2, ymm0, ymm1; vmovdqu ymmword ptr [rdi+128], ymm2",
            "vmovdqu ymm0, ymmword ptr [rip + data_a]; vmovdqu ymm1, ymmword ptr [rip + data_b]; vpshufb ymm0, ymm0, ymm1; vmovdqu ymmword ptr [rdi+160], ymm0",
            "vmovdqu ymm0, ymmword ptr [rip + data_a]; vmovdqu ymm1, ymmword ptr [rip + data_b]; vpshufb ymm1, ymm0, ymm1; vmovdqu ymmword ptr [rdi+192], ymm1",
            "vmovdqu ymm0, ymmword ptr [rip + data_a]; vpshufb ymm2, ymm0, ymmword ptr [rip + data_b]; vmovdqu ymmword ptr [rdi+224], ymm2",
        ]
        fault = "vmovdqu ymm0, ymmword ptr [rip + data_a]; vpshufb ymm1, ymm0, ymmword ptr [rdi]"
    else:
        cases = []
        for width, base in (("xmm", 0), ("ymm", 128)):
            cases.extend([
                f"{'movdqu' if width == 'xmm' else 'vmovdqu'} {width}0, {width}word ptr [rip + data_a]; {op} {width}2, {width}0, 0x00; vmovdqu ymmword ptr [rdi+{base}], ymm2",
                f"{'movdqu' if width == 'xmm' else 'vmovdqu'} {width}0, {width}word ptr [rip + data_a]; {op} {width}0, {width}0, 0x1b; vmovdqu ymmword ptr [rdi+{base+32}], ymm0",
                f"{'movdqu' if width == 'xmm' else 'vmovdqu'} {width}0, {width}word ptr [rip + data_a]; {op} {width}1, {width}0, 0xff; vmovdqu ymmword ptr [rdi+{base+64}], ymm1",
                f"{op} {width}2, {width}word ptr [rip + data_b], 0xff; vmovdqu ymmword ptr [rdi+{base+96}], ymm2",
            ])
        fault = f"vmovdqu ymm0, ymmword ptr [rip + data_a]; {op} ymm1, ymmword ptr [rdi], 0xff"
    for case in cases:
        for statement in case.split("; "):
            lines.append("    " + statement)
    lines += ["    vzeroupper", "    ret", "", "    .globl latx_avx_single_" + op + "_fault", "    .type latx_avx_single_" + op + "_fault, @function", "latx_avx_single_" + op + "_fault:"]
    lines.extend("    " + statement for statement in fault.split("; "))
    lines += ["    ret", DATA, "    .section .note.GNU-stack,\"\",@progbits", ""]
    return "\n".join(lines)


C_SOURCE = r'''/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"

enum { SYS_MMAP = 9, SYS_MPROTECT = 10, PROT_READ = 1, PROT_WRITE = 2,
       PROT_NONE = 0, MAP_PRIVATE = 2, MAP_ANONYMOUS = 0x20,
       PAGE = 4096, OUTPUT_SIZE = 256 };

extern void latx_avx_single___MNEMONIC___run(uint8_t *);
extern void latx_avx_single___MNEMONIC___fault(uint8_t *);
static uint8_t output[OUTPUT_SIZE];

static inline long syscall6(long n, long a, long b, long c, long d, long e, long f)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a;
    register long rsi __asm__("rsi") = b;
    register long rdx __asm__("rdx") = c;
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8") = e;
    register long r9 __asm__("r9") = f;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx),
                     "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return rax;
}

int latx_avx_single_main(long argc, char **argv)
{
    if (argc <= 1 || (argc == 2 && argv[1][0] == 'n')) {
        latx_avx_single___MNEMONIC___run(output);
        return latx_avx_single_write_all(output, sizeof(output)) != 0;
    }
    if (argc == 2 && argv[1][0] == 'f') {
        long mapping = syscall6(SYS_MMAP, 0, PAGE * 2, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapping < 0) return 71;
        if (latx_avx_single_syscall3(SYS_MPROTECT, mapping + PAGE, PAGE,
                                     PROT_NONE) < 0) return 72;
        latx_avx_single___MNEMONIC___fault((uint8_t *)(mapping + PAGE - 16));
        return 90;
    }
    return 2;
}
'''


RUNNER = r'''#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 2 ]]; then
    echo "usage: $0 xzy86 OUTPUT_DIR" >&2
    exit 2
fi
host=$1
out=$2
mnemonic=__MNEMONIC__
stem="latx-avx-single-$mnemonic"
root=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p "$out"
remote_dir="/tmp/wi1919-$mnemonic-$(date +%s)-$$"
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
ssh "${ssh_args[@]}" "$host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$root/tests/integration/latx-avx-single-runtime.S" \
    "$root/tests/integration/$stem.S" "$root/tests/integration/$stem.c" \
    "$root/tests/integration/latx-avx-single-common.h" "$host:$remote_dir/"
ssh "${ssh_args[@]}" "$host" bash -s -- "$remote_dir" "$stem" <<'REMOTE'
set -euo pipefail
remote_dir=$1
stem=$2
docker run --rm --env STEM="$stem" -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
  gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
      -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
      -mgeneral-regs-only -mno-avx -mno-avx2 -I /work \
      -o "/work/$STEM.static" /work/latx-avx-single-runtime.S \
      "/work/$STEM.S" "/work/$STEM.c"
  objdump -d -Mintel "/work/$STEM.static" > "/work/$STEM.objdump"
'
REMOTE
scp "${scp_args[@]}" "$host:$remote_dir/$stem.static" "$out/$stem.static"
scp "${scp_args[@]}" "$host:$remote_dir/$stem.objdump" "$out/$stem.objdump"
run_case() {
    local case_name=$1
    local out_name=$2
    set +e
    ssh "${ssh_args[@]}" "$host" "set +e; $remote_dir/$stem.static $case_name >$remote_dir/$out_name.stdout 2>$remote_dir/$out_name.stderr; printf '%s\\n' \$? >$remote_dir/$out_name.exit; exit 0" \
        >"$out/$out_name.ssh.stdout" 2>"$out/$out_name.ssh.stderr"
    scp "${scp_args[@]}" "$host:$remote_dir/$out_name.stdout" "$out/$out_name.stdout"
    scp "${scp_args[@]}" "$host:$remote_dir/$out_name.stderr" "$out/$out_name.stderr"
    scp "${scp_args[@]}" "$host:$remote_dir/$out_name.exit" "$out/$out_name.exit"
    local rc
    rc=$(cat "$out/$out_name.exit")
    set -e
    printf 'remote_native_exit=%s\n' "$rc" >"$out/$out_name.status"
}
run_case normal x86-normal
run_case fault x86-fault
[[ "$(cat "$out/x86-normal.exit")" == 0 ]]
[[ "$(cat "$out/x86-fault.exit")" == 139 ]]
sha256sum "$out/$stem.static" "$out/$stem.objdump" \
    "$out/x86-normal.stdout" "$out/x86-normal.stderr" \
    "$out/x86-fault.stdout" "$out/x86-fault.stderr" >"$out/sha256.txt"
cat >"$out/manifest.json" <<EOF
{
  "work_item": "WI-1919",
  "mnemonic": "$mnemonic",
  "baseline": "xzy86-native",
  "normal": {"command": "ssh $host $remote_dir/$stem.static normal", "exit": 0},
  "fault": {"command": "ssh $host $remote_dir/$stem.static fault", "expected_exit": 139},
  "objdump": "$out/$stem.objdump",
  "sha256": "$out/sha256.txt",
  "coverage": ["XMM", "YMM", "register", "memory", "alias", "VEX.128 high-half clear", "cross-page fault"]
}
EOF
printf 'PASS WI-1919 xzy86 native fixture: %s\n' "$mnemonic"
'''


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    integration = args.repo / "tests/integration"
    for mnemonic in MNEMONICS:
        stem = f"latx-avx-single-{mnemonic}"
        (integration / f"{stem}.S").write_text(assembly(mnemonic))
        (integration / f"{stem}.c").write_text(C_SOURCE.replace("__MNEMONIC__", mnemonic))
        (integration / f"test-latx-avx-wi1919-{mnemonic}-x86.sh").write_text(
            RUNNER.replace("__MNEMONIC__", mnemonic)
        )
        (integration / f"test-latx-avx-wi1919-{mnemonic}-x86.sh").chmod(0o755)
    manifest = {
        "schema": 1,
        "work_item": "WI-1919",
        "mnemonics": list(MNEMONICS),
        "fixtures": {
            mnemonic: {
                "assembly": f"tests/integration/latx-avx-single-{mnemonic}.S",
                "c": f"tests/integration/latx-avx-single-{mnemonic}.c",
                "runner": f"tests/integration/test-latx-avx-wi1919-{mnemonic}-x86.sh",
                "normal_records": 8,
                "fault_case": "cross-page 256-bit memory source",
                "coverage": ["XMM", "YMM", "register", "memory", "alias", "VEX.128 high-half clear"],
                "immediates": ["0x00", "0x1b", "0xff"] if mnemonic != "vpshufb" else ["control-byte high-bit set"],
            }
            for mnemonic in MNEMONICS
        },
        "runtime_boundary": "xzy86 native only; no Ninja, build64.sh, configure, LATX, LASX, or LSX",
    }
    manifest_path = integration / "wi1919-fixture-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    if args.check:
        required = []
        for mnemonic in MNEMONICS:
            stem = f"latx-avx-single-{mnemonic}"
            required += [integration / f"{stem}.S", integration / f"{stem}.c", integration / f"test-latx-avx-wi1919-{mnemonic}-x86.sh"]
        missing = [str(path) for path in required if not path.is_file()]
        if missing:
            raise SystemExit("missing generated files: " + ", ".join(missing))
        print("PASS WI-1919 generated files: " + ", ".join(MNEMONICS))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
