#!/usr/bin/env python3

import collections
import re
import sys


EFLAGS_MASK = 0x8D5


def read_qemu(path):
    states = []
    pending_pc = None
    with open(path, errors="replace") as stream:
        for line in stream:
            match = re.search(r"/([0-9a-fA-F]{16})/", line)
            if line.startswith("Trace ") and match:
                pending_pc = int(match.group(1), 16)
                continue
            match = re.search(
                r"RIP=[0-9a-fA-F]+ RFL=([0-9a-fA-F]+)", line)
            if match and pending_pc is not None:
                states.append((pending_pc, int(match.group(1), 16)))
                pending_pc = None
    return states


def read_latx(path):
    states = []
    pattern = re.compile(
        r"LATX_TB_STATE cpu=\d+ pc=0x([0-9a-fA-F]+) "
        r"eflags=0x([0-9a-fA-F]+)")
    with open(path, errors="replace") as stream:
        for line in stream:
            match = pattern.search(line)
            if match:
                states.append((int(match.group(1), 16),
                               int(match.group(2), 16)))
    return states


def main():
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} QEMU_LOG LATX_LOG")

    qemu = read_qemu(sys.argv[1])
    latx = read_latx(sys.argv[2])
    if len(qemu) != len(latx):
        raise SystemExit(
            f"TB count differs: QEMU={len(qemu)} LATX={len(latx)}")

    flag_differences = collections.Counter()
    for index, (qemu_state, latx_state) in enumerate(zip(qemu, latx)):
        qemu_pc, qemu_flags = qemu_state
        latx_pc, latx_flags = latx_state
        if qemu_pc != latx_pc:
            previous = index - 1
            if previous >= 0:
                q_prev_pc, q_prev_flags = qemu[previous]
                l_prev_pc, l_prev_flags = latx[previous]
                print(f"previous TB QEMU pc={q_prev_pc:#x} "
                      f"eflags={q_prev_flags:#x}; LATX pc={l_prev_pc:#x} "
                      f"eflags={l_prev_flags:#x}")
            raise SystemExit(
                f"control-flow divergence at TB {index}: "
                f"QEMU={qemu_pc:#x} LATX={latx_pc:#x}")
        difference = (qemu_flags ^ latx_flags) & EFLAGS_MASK
        if difference:
            flag_differences[difference] += 1

    details = ", ".join(
        f"{mask:#x}:{count}" for mask, count in sorted(flag_differences.items()))
    if not details:
        details = "none"
    print(f"PASS TB control flow: {len(qemu)} entries, 0 divergent branches")
    print(f"non-control-flow EFLAGS differences: {details}")


if __name__ == "__main__":
    main()
