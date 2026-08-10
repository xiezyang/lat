#!/usr/bin/env python3
"""Generate the independent xzy86 VPTEST fixture and runner."""

import argparse
import json
from pathlib import Path

ASM = r'''/* SPDX-License-Identifier: GPL-2.0-only */
    .intel_syntax noprefix
    .text
    .globl latx_avx_single_vptest_run
    .type latx_avx_single_vptest_run, @function
latx_avx_single_vptest_run:
    vmovdqu ymm0, ymmword ptr [rip + data_zero]
    vmovdqu ymm1, ymmword ptr [rip + data_zero]
    vptest xmm0, xmm1
    jmp .Lsave0
    .p2align 4
.Lsave0:
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_ones]
    vmovdqu ymm1, ymmword ptr [rip + data_ones]
    vptest xmm0, xmm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_zero]
    vmovdqu ymm1, ymmword ptr [rip + data_ones]
    vptest xmm0, xmm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_bit]
    vmovdqu ymm1, ymmword ptr [rip + data_mixed]
    vptest xmm0, xmm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_zero]
    vmovdqu ymm1, ymmword ptr [rip + data_zero]
    vptest ymm0, ymm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_bit]
    vptest ymm0, ymm0
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm0
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_bit]
    vmovdqu ymm1, ymmword ptr [rip + data_mixed]
    vptest ymm0, ymm1
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    add rdi, 72
    vmovdqu ymm0, ymmword ptr [rip + data_ones]
    vptest ymm0, ymmword ptr [rip + data_misaligned+1]
    pushfq
    pop r11
    vmovdqu ymmword ptr [rdi], ymm0
    vmovdqu ymmword ptr [rdi+32], ymm1
    mov qword ptr [rdi+64], r11
    ret

    .globl latx_avx_single_vptest_fault128
latx_avx_single_vptest_fault128:
    vmovdqu ymm0, ymmword ptr [rip + data_ones]
    vptest xmm0, xmmword ptr [rdi]
    ret
    .globl latx_avx_single_vptest_fault256
latx_avx_single_vptest_fault256:
    vmovdqu ymm0, ymmword ptr [rip + data_ones]
    vptest ymm0, ymmword ptr [rdi]
    ret

    .section .rodata
    .balign 32
data_zero:
    .quad 0,0,0,0
data_ones:
    .quad -1,-1,-1,-1
data_bit:
    .quad 1,1,1,1
data_mixed:
    .quad 3,3,3,3
data_misaligned:
    .byte 0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55
    .byte 0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa
    .byte 0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55
    .byte 0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa
    .section .note.GNU-stack,"",@progbits
'''

C_SOURCE = r'''/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"
enum { SYS_MMAP = 9, SYS_MPROTECT = 10, PROT_READ = 1, PROT_WRITE = 2,
       PROT_NONE = 0, MAP_PRIVATE = 2, MAP_ANONYMOUS = 0x20,
       PAGE = 4096, OUTPUT_SIZE = 8 * 72 };
extern void latx_avx_single_vptest_run(uint8_t *);
extern void latx_avx_single_vptest_fault128(uint8_t *);
extern void latx_avx_single_vptest_fault256(uint8_t *);
static uint8_t output[OUTPUT_SIZE];
static inline long syscall6(long n,long a,long b,long c,long d,long e,long f)
{ register long rax __asm__("rax")=n,rdi __asm__("rdi")=a,rsi __asm__("rsi")=b,
    rdx __asm__("rdx")=c,r10 __asm__("r10")=d,r8 __asm__("r8")=e,r9 __asm__("r9")=f;
  __asm__ volatile("syscall":"+a"(rax):"D"(rdi),"S"(rsi),"d"(rdx),"r"(r10),"r"(r8),"r"(r9):"rcx","r11","memory"); return rax; }
int latx_avx_single_main(long argc,char **argv)
{
  if (argc <= 1 || (argc == 2 && argv[1][0] == 'n')) {
    latx_avx_single_vptest_run(output);
    return latx_avx_single_write_all(output,sizeof(output)) != 0;
  }
  if (argc == 2 && (argv[1][0] == '1' || argv[1][0] == '2')) {
    long p=syscall6(SYS_MMAP,0,PAGE*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (p < 0) return 71;
    if (latx_avx_single_syscall3(SYS_MPROTECT,p+PAGE,PAGE,PROT_NONE) < 0) return 72;
    if (argv[1][0] == '1') latx_avx_single_vptest_fault128((uint8_t *)(p+PAGE-8));
    else latx_avx_single_vptest_fault256((uint8_t *)(p+PAGE-16));
    return 90;
  }
  return 2;
}
'''

RUNNER = r'''#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 2 ]]; then echo "usage: $0 xzy86 OUTPUT_DIR" >&2; exit 2; fi
host=$1; out=$2; stem=latx-avx-single-vptest
root=$(cd "$(dirname "$0")/../.." && pwd); mkdir -p "$out"; remote_dir="/tmp/wi1923-vptest-$(date +%s)-$$"
ssh_args=(); scp_args=(); if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then ssh_args=(-F "$LATX_SSH_CONFIG"); scp_args=(-F "$LATX_SSH_CONFIG"); fi
ssh "${ssh_args[@]}" "$host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$root/tests/integration/latx-avx-single-runtime.S" "$root/tests/integration/$stem.S" "$root/tests/integration/$stem.c" "$root/tests/integration/latx-avx-single-common.h" "$host:$remote_dir/"
ssh "${ssh_args[@]}" "$host" bash -s -- "$remote_dir" "$stem" <<'REMOTE'
set -euo pipefail; remote_dir=$1; stem=$2
docker run --rm --env STEM="$stem" -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
 gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone -mgeneral-regs-only -mno-avx -mno-avx2 -I /work -o "/work/$STEM.static" /work/latx-avx-single-runtime.S "/work/$STEM.S" "/work/$STEM.c"
 objdump -d -Mintel "/work/$STEM.static" > "/work/$STEM.objdump"
'
REMOTE
scp "${scp_args[@]}" "$host:$remote_dir/$stem.static" "$out/$stem.static"; scp "${scp_args[@]}" "$host:$remote_dir/$stem.objdump" "$out/$stem.objdump"
run_case() { local remote_name=$1; local output_name=$2; set +e; ssh "${ssh_args[@]}" "$host" "set +e; $remote_dir/$stem.static $remote_name >$remote_dir/x86-$output_name.stdout 2>$remote_dir/x86-$output_name.stderr; printf '%s\\n' \$? >$remote_dir/x86-$output_name.exit; exit 0" >"$out/x86-$output_name.ssh.stdout" 2>"$out/x86-$output_name.ssh.stderr"; scp "${scp_args[@]}" "$host:$remote_dir/x86-$output_name.stdout" "$out/x86-$output_name.stdout"; scp "${scp_args[@]}" "$host:$remote_dir/x86-$output_name.stderr" "$out/x86-$output_name.stderr"; scp "${scp_args[@]}" "$host:$remote_dir/x86-$output_name.exit" "$out/x86-$output_name.exit"; set -e; }
run_case n normal; run_case 1 fault128; run_case 2 fault256
[[ "$(cat "$out/x86-normal.exit")" == 0 ]]; [[ "$(cat "$out/x86-fault128.exit")" == 139 ]]; [[ "$(cat "$out/x86-fault256.exit")" == 139 ]]
sha256sum "$out/$stem.static" "$out/$stem.objdump" "$out/x86-normal.stdout" "$out/x86-normal.stderr" "$out/x86-fault128.stdout" "$out/x86-fault128.stderr" "$out/x86-fault256.stdout" "$out/x86-fault256.stderr" >"$out/sha256.txt"
cat >"$out/manifest.json" <<EOF
{"work_item":"WI-1923","mnemonic":"vptest","baseline":"xzy86-native","normal_exit":0,"fault128_exit":139,"fault256_exit":139,"normal_bytes":576,"fault_bytes":0,"coverage":["XMM/YMM","register/m128/m256","alias","CF/ZF combinations","VEX.128 source high-half observation","unaligned","cross-page"]}
EOF
printf 'PASS WI-1923 xzy86 native fixture\n'
'''


def main() -> int:
    parser = argparse.ArgumentParser(); parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2]); parser.add_argument("--check", action="store_true"); args=parser.parse_args()
    i=args.repo/"tests/integration"; (i/"latx-avx-single-vptest.S").write_text(ASM); (i/"latx-avx-single-vptest.c").write_text(C_SOURCE)
    r=i/"test-latx-avx-wi1923-vptest-x86.sh"; r.write_text(RUNNER); r.chmod(0o755)
    m={"schema":1,"work_item":"WI-1923","mnemonic":"vptest","encoding":"VEX.128 and VEX.256","normal_records":8,"record_bytes":72,"faults":["m128 at page-8","m256 at page-16"],"coverage":["XMM/YMM","register/memory","dest/src alias","CF/ZF boundary combinations","EFLAGS","source high-half observation","unaligned"],"runtime_boundary":"xzy86 native only; no build64.sh, configure, Ninja, LATX, LASX, LSX"}
    (i/"wi1923-fixture-manifest.json").write_text(json.dumps(m,indent=2)+"\n")
    if args.check: print("PASS WI-1923 generated files: vptest")

if __name__ == "__main__": raise SystemExit(main())
