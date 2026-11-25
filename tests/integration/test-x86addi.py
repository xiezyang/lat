#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise signed immediate ADD flags through a 6600-capable LAT runtime."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile


def flags(a, b, bits):
    mask = (1 << bits) - 1
    a, b = a & mask, b & mask
    result = (a + b) & mask
    return (int(a + b > mask) | (int(bin(result & 255).count('1') % 2 == 0) << 2)
            | (int(bool((a ^ b ^ result) & 16)) << 4)
            | (int(result == 0) << 6) | ((result >> (bits - 1)) << 7)
            | (int(bool((~(a ^ b) & (a ^ result)) >> (bits - 1) & 1)) << 11))


def fixture(abi):
    wide = abi == 'x86_64'
    full, temp, pointer = ('rax', 'rdx', 'rcx') if wide else ('eax', 'edx', 'ecx')
    registers = {8: 'al', 16: 'ax', 32: 'eax', 64: 'rax'}
    sizes = {8: 'byte', 16: 'word', 32: 'dword', 64: 'qword'}
    lines = ['BITS ' + ('64' if wide else '32'), 'global _start', 'section .text',
             '_start:']
    count = 0

    def add_case(bits, immediate, initial, memory=False, seed=0, force_wide=False, locked=False):
        nonlocal count
        mask = (1 << bits) - 1
        result = ((initial & mask) + immediate) & mask
        expected = result if bits == 64 or (bits == 32 and not memory) else (
            (initial & ~mask) | result)
        lines.append(f'mov {full}, {initial}')
        if memory:
            lines.append(f'mov [{"rel " if wide else ""}scratch], {full}')
            operand = f'{sizes[bits]} [{"rel " if wide else ""}scratch]'
        else:
            operand = registers[bits]
        lines.append('stc' if seed else 'clc')
        size = ('strict qword ' if bits == 64 else 'strict dword ') if force_wide else ''
        operation = 'lock add' if locked else 'add'
        lines.extend([f'{operation} {operand}, {size}{immediate}', 'pushf', f'pop {pointer}',
                      'and ecx, 0x8d5', f'cmp ecx, {flags(initial, immediate, bits)}',
                      'jne fail'])
        if memory:
            lines.append(f'mov {full}, [{"rel " if wide else ""}scratch]')
        lines.extend([f'mov {temp}, {expected}', f'cmp {full}, {temp}', 'jne fail'])
        count += 1

    for bits in ([32, 64] if wide else [32]):
        mask, sign = (1 << bits) - 1, 1 << (bits - 1)
        for immediate in range(-128, 128):
            random = ((immediate + 137) * 6364136223846793005 + 1442695040888963407) & mask
            for value in [0, 1, 15, sign - 1, sign, mask, random]:
                initial = value | (0xabcdef1200000000 if wide and bits == 32 else 0)
                for memory in [False, True]:
                    for seed in [0, 1]:
                        add_case(bits, immediate, initial, memory, seed)
        for immediate in [-2049, -129, 128, 255, 2048]:
            for memory in [False, True]:
                add_case(bits, immediate, mask, memory)
        for immediate in [-128, -1, 127]:
            add_case(bits, immediate, mask, force_wide=True)
        for immediate in [-128, -1, 0, 1, 127]:
            add_case(bits, immediate, mask, memory=True, locked=True)
    # Preserve the real stack while checking ADD with ESP/RSP as its input.
    stack = 'rsp' if wide else 'esp'
    saved = 'rbx' if wide else 'ebx'
    for bits in ([32, 64] if wide else [32]):
        for initial, immediate in [((1 << (bits - 1)) - 1, 1),
                                   ((1 << bits) - 1, 1), (0, -1)]:
            result = (initial + immediate) & ((1 << bits) - 1)
            operand = 'rsp' if bits == 64 else 'esp'
            lines.extend([f'mov {saved}, {stack}', f'mov {stack}, {initial}',
                          f'add {operand}, {immediate}', f'mov {temp}, {stack}',
                          f'mov {stack}, {saved}', 'pushf', f'pop {pointer}',
                          'and ecx, 0x8d5', f'cmp ecx, {flags(initial, immediate, bits)}',
                          'jne fail', f'mov {full}, {result}',
                          f'cmp {temp}, {full}', 'jne fail'])
            count += 1
    for bits in [8, 16]:
        for immediate in [-128, -1, 0, 127]:
            for memory in [False, True]:
                add_case(bits, immediate, 0xabcdef12ffffffff if wide else 0xffffffff,
                         memory)
    # Dead ADD flags and the different CF rules of ADC/INC keep their old paths.
    lines.extend(['mov eax, 7', 'add eax, -1', 'cmp eax, 6', 'jne fail',
                  'mov eax, 0xffffffff', 'stc', 'adc eax, 0',
                  'jnc fail', 'cmp eax, 0', 'jne fail',
                  'mov eax, 0', 'stc', 'inc eax', 'jnc fail',
                  'cmp eax, 1', 'jne fail',
                  'mov eax, 0', 'sub eax, 1', 'jnc fail',
                  'cmp eax, -1', 'jne fail',
                  'mov eax, 0xffffffff', 'mov edx, 1', 'xadd eax, edx',
                  'jnc fail', 'cmp eax, 0', 'jne fail',
                  'cmp edx, -1', 'jne fail'])
    if wide:
        lines.extend(['mov eax, 60', 'xor edi, edi', 'syscall',
                      'fail:', 'mov eax, 60', 'mov edi, 1', 'syscall'])
    else:
        lines.extend(['mov eax, 1', 'xor ebx, ebx', 'int 0x80',
                      'fail:', 'mov eax, 1', 'mov ebx, 1', 'int 0x80'])
    lines.extend(['section .bss', 'align 8', 'scratch: resq 1'])
    return '\n'.join(lines) + '\n', count


def main():
    if os.environ.get('LATX_TEST_X86ADDI') != '1':
        print('SKIP: LATX_TEST_X86ADDI=1 requires a runtime supporting x86addi.w/d')
        return 77
    if not all(shutil.which(tool) for tool in ('nasm', 'ld.lld')):
        print('SKIP: nasm and ld.lld are required')
        return 77
    emulator, abi = str(Path(sys.argv[1]).resolve()), sys.argv[2]
    if abi not in ('x86_64', 'i386'):
        raise ValueError(abi)
    source, count = fixture(abi)
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)
        (work / 'guest.asm').write_text(source)
        subprocess.run(['nasm', '-f', 'elf64' if abi == 'x86_64' else 'elf32',
                        str(work / 'guest.asm'), '-o', str(work / 'guest.o')], check=True)
        guest = work / 'guest'
        subprocess.run(['ld.lld', '-static', '-m', 'elf_x86_64' if abi == 'x86_64'
                        else 'elf_i386', str(work / 'guest.o'), '-o', str(guest)], check=True)
        env = {key: value for key, value in os.environ.items() if not key.startswith('LATX_')}
        env.update(LATX_AOT='0', LATX_TU='0', LATX_KZT='0', LATX_MT='0',
                   LATX_HASH='0', LATX_SOFTFPU='0', LATX_SOFTFPU_FAST='0',
                   LATX_FLOAT80='0', LATX_STACK='0', LATX_128CAL='0')
        command = shlex.split(os.environ.get('LATX_TEST_LA_RUNNER', ''))
        command += [emulator, '-latx-softfpu', '0', '-latx-softfpu-fast', '0', str(guest)]
        for name, setting, cli in [('default', None, []),
                                   ('off', '0', []), ('on', '1', []),
                                   ('cli-off', '1', ['-latx-x86addi', '0']),
                                   ('cli-on', '0', ['-latx-x86addi', '1'])]:
            run_env = dict(env)
            if setting is not None:
                run_env['LATX_X86ADDI'] = setting
            subprocess.run(command[:-1] + cli + command[-1:], env=run_env,
                           check=True, timeout=110)
            print(f'PASS {abi}: x86addi {name}', flush=True)
    print(f'PASS {abi}: {count} ADD cases per mode and ADC/INC/SUB/XADD/dead-flag guards; softfpu=0 fast=0')
    return 0


if __name__ == '__main__':
    sys.exit(main())
