#!/usr/bin/env python3
"""Generate independent xzy86 fixtures for the four AVX string compares."""

import argparse
import json
from pathlib import Path

MNEMONICS = ("vpcmpestri", "vpcmpestrm", "vpcmpistri", "vpcmpistrm")

DATA = r'''
    .section .rodata
    .balign 16
data_a:
    .byte 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07
    .byte 0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
data_b:
    .byte 0x00,0x80,0x7f,0xff,0x01,0x02,0x03,0x04
    .byte 0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c
'''


def capture(kind: str) -> list[str]:
    lines = ["    pushfq", "    pop r11"]
    if kind == "mask":
        lines += ["    movdqu xmmword ptr [rdi], xmm0", "    mov qword ptr [rdi+16], r11", "    mov dword ptr [rdi+24], eax", "    mov dword ptr [rdi+28], edx", "    mov dword ptr [rdi+32], ecx"]
    else:
        lines += ["    mov qword ptr [rdi], rcx", "    mov qword ptr [rdi+8], r11", "    mov dword ptr [rdi+16], eax", "    mov dword ptr [rdi+20], edx"]
    lines += ["    add rdi, 64"]
    return lines


def instruction_cases(mnemonic: str) -> tuple[list[list[str]], list[str]]:
    is_mask = mnemonic.endswith("m")
    explicit = mnemonic.startswith("vpcmpestr")
    kind = "mask" if is_mask else "index"
    cases = []

    def add(lines: list[str]):
        cases.append(lines + capture(kind))

    def lengths(eax: int, edx: int) -> list[str]:
        if explicit:
            return [f"    mov eax, {eax}", f"    mov edx, {edx}"]
        return [f"    mov ecx, {eax}", f"    mov edx, {edx}"]

    op = mnemonic
    add(["    movdqu xmm0, xmmword ptr [rip + data_a]", "    movdqu xmm1, xmmword ptr [rip + data_b]", *lengths(16, 16), f"    {op} xmm0, xmm1, 0x00"])
    add(["    movdqu xmm0, xmmword ptr [rip + data_a]", "    movdqu xmm1, xmmword ptr [rip + data_b]", *lengths(0, 0), f"    {op} xmm0, xmm1, 0x05"])
    add(["    movdqu xmm0, xmmword ptr [rip + data_a]", "    movdqu xmm1, xmmword ptr [rip + data_b]", *lengths(8, 8), f"    {op} xmm0, xmm1, 0x0a"])
    add(["    movdqu xmm0, xmmword ptr [rip + data_a]", "    movdqu xmm1, xmmword ptr [rip + data_b]", *lengths(16, 8), f"    {op} xmm0, xmm1, 0x1f"])
    add(["    movdqu xmm0, xmmword ptr [rip + data_a]", *lengths(16, 16), f"    {op} xmm0, xmm0, 0x2d"])
    add(["    movdqu xmm0, xmmword ptr [rip + data_a]", *lengths(16, 16), f"    {op} xmm0, xmmword ptr [rip + data_b], 0x37"])
    add(["    movdqu xmm2, xmmword ptr [rip + data_a]", "    movdqu xmm1, xmmword ptr [rip + data_b]", *lengths(15, 1), f"    {op} xmm2, xmm1, 0x4b"])
    add(["    movdqu xmm0, xmmword ptr [rip + data_b]", *lengths(1, 15), f"    {op} xmm0, xmmword ptr [rip + data_a], 0x5f"])
    fault = ["    movdqu xmm0, xmmword ptr [rip + data_a]", *lengths(16, 16), f"    {op} xmm0, xmmword ptr [rdi], 0x00"]
    return cases, fault


def assembly(mnemonic: str) -> str:
    cases, fault = instruction_cases(mnemonic)
    lines = ["/* SPDX-License-Identifier: GPL-2.0-only */", "    .intel_syntax noprefix", "    .text", f"    .globl latx_avx_single_{mnemonic}_run", f"    .type latx_avx_single_{mnemonic}_run, @function", f"latx_avx_single_{mnemonic}_run:"]
    for case in cases:
        lines.extend(case)
    lines += ["    ret", "", f"    .globl latx_avx_single_{mnemonic}_fault", f"    .type latx_avx_single_{mnemonic}_fault, @function", f"latx_avx_single_{mnemonic}_fault:"]
    lines.extend(fault)
    lines += ["    ret", DATA, "    .section .note.GNU-stack,\"\",@progbits", ""]
    return "\n".join(lines)


C_SOURCE = r'''/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"
enum { SYS_MMAP = 9, SYS_MPROTECT = 10, PROT_READ = 1, PROT_WRITE = 2,
       PROT_NONE = 0, MAP_PRIVATE = 2, MAP_ANONYMOUS = 0x20,
       PAGE = 4096, OUTPUT_SIZE = 512 };
extern void latx_avx_single___MNEMONIC___run(uint8_t *);
extern void latx_avx_single___MNEMONIC___fault(uint8_t *);
static uint8_t output[OUTPUT_SIZE];
static inline long syscall6(long n, long a, long b, long c, long d, long e, long f)
{
    register long rax __asm__("rax") = n, rdi __asm__("rdi") = a,
        rsi __asm__("rsi") = b, rdx __asm__("rdx") = c,
        r10 __asm__("r10") = d, r8 __asm__("r8") = e, r9 __asm__("r9") = f;
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
        latx_avx_single___MNEMONIC___fault((uint8_t *)(mapping + PAGE - 8));
        return 90;
    }
    return 2;
}
'''


RUNNER = r'''#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 2 ]]; then echo "usage: $0 xzy86 OUTPUT_DIR" >&2; exit 2; fi
host=$1; out=$2; mnemonic=__MNEMONIC__; stem="latx-avx-single-$mnemonic"
root=$(cd "$(dirname "$0")/../.." && pwd); mkdir -p "$out"
remote_dir="/tmp/wi1921-$mnemonic-$(date +%s)-$$"
ssh_args=(); scp_args=();
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then ssh_args=(-F "$LATX_SSH_CONFIG"); scp_args=(-F "$LATX_SSH_CONFIG"); fi
ssh "${ssh_args[@]}" "$host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$root/tests/integration/latx-avx-single-runtime.S" "$root/tests/integration/$stem.S" "$root/tests/integration/$stem.c" "$root/tests/integration/latx-avx-single-common.h" "$host:$remote_dir/"
ssh "${ssh_args[@]}" "$host" bash -s -- "$remote_dir" "$stem" <<'REMOTE'
set -euo pipefail
remote_dir=$1; stem=$2
docker run --rm --env STEM="$stem" -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
  gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone -mgeneral-regs-only -mno-avx -mno-avx2 -I /work -o "/work/$STEM.static" /work/latx-avx-single-runtime.S "/work/$STEM.S" "/work/$STEM.c"
  objdump -d -Mintel "/work/$STEM.static" > "/work/$STEM.objdump"
'
REMOTE
scp "${scp_args[@]}" "$host:$remote_dir/$stem.static" "$out/$stem.static"
scp "${scp_args[@]}" "$host:$remote_dir/$stem.objdump" "$out/$stem.objdump"
run_case() {
    local name=$1; set +e
    ssh "${ssh_args[@]}" "$host" "set +e; $remote_dir/$stem.static $name >$remote_dir/x86-$name.stdout 2>$remote_dir/x86-$name.stderr; printf '%s\\n' \$? >$remote_dir/x86-$name.exit; exit 0" >"$out/x86-$name.ssh.stdout" 2>"$out/x86-$name.ssh.stderr"
    scp "${scp_args[@]}" "$host:$remote_dir/x86-$name.stdout" "$out/x86-$name.stdout"
    scp "${scp_args[@]}" "$host:$remote_dir/x86-$name.stderr" "$out/x86-$name.stderr"
    scp "${scp_args[@]}" "$host:$remote_dir/x86-$name.exit" "$out/x86-$name.exit"
    set -e
}
run_case normal; run_case fault
[[ "$(cat "$out/x86-normal.exit")" == 0 ]]; [[ "$(cat "$out/x86-fault.exit")" == 139 ]]
sha256sum "$out/$stem.static" "$out/$stem.objdump" "$out/x86-normal.stdout" "$out/x86-normal.stderr" "$out/x86-fault.stdout" "$out/x86-fault.stderr" >"$out/sha256.txt"
cat >"$out/manifest.json" <<EOF
{"work_item":"WI-1921","mnemonic":"$mnemonic","baseline":"xzy86-native","normal_exit":0,"fault_exit":139,"normal_bytes":512,"fault_bytes":0,"objdump":"$out/$stem.objdump","sha256":"$out/sha256.txt","forms":["XMM register","m128 memory","alias","explicit/implicit length","EFLAGS","ECX index or XMM0 mask"]}
EOF
printf 'PASS WI-1921 xzy86 native fixture: %s\n' "$mnemonic"
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
        (integration / f"{stem}.c").write_text(C_SOURCE.replace("___MNEMONIC___", "_" + mnemonic + "_"))
        runner = integration / f"test-latx-avx-wi1921-{mnemonic}-x86.sh"
        runner.write_text(RUNNER.replace("__MNEMONIC__", mnemonic)); runner.chmod(0o755)
    manifest = {
        "schema": 1, "work_item": "WI-1921", "mnemonics": list(MNEMONICS),
        "encoding_boundary": "XMM-only VEX.128; no legal YMM form",
        "fixtures": {m: {"assembly": f"tests/integration/latx-avx-single-{m}.S", "c": f"tests/integration/latx-avx-single-{m}.c", "runner": f"tests/integration/test-latx-avx-wi1921-{m}-x86.sh", "normal_records": 8, "normal_bytes": 512, "fault": "m128 source crossing page boundary", "coverage": ["imm8 controls", "byte/word", "signed/unsigned", "length zero/max/boundary", "EFLAGS", "ECX index or XMM0 mask", "register/memory", "alias"]} for m in MNEMONICS},
        "runtime_boundary": "xzy86 native only; no build64.sh, configure, Ninja, LATX, LASX, or LSX",
    }
    (integration / "wi1921-fixture-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if args.check:
        for m in MNEMONICS:
            for suffix in (".S", ".c", f".sh"):
                p = integration / (f"latx-avx-single-{m}{suffix}" if suffix != ".sh" else f"test-latx-avx-wi1921-{m}-x86.sh")
                if not p.is_file(): raise SystemExit(f"missing {p}")
        print("PASS WI-1921 generated files: " + ", ".join(MNEMONICS))


if __name__ == "__main__":
    raise SystemExit(main())
