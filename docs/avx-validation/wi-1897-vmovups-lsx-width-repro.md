# WI-1897 VMOVUPS LSX Width Reproduction

This record is a minimal reproduction of the VMOVUPS LSX mismatch found before
the source-session matrix was allowed to continue. It uses the independent
`register-reference` fixture and does not change the LASX translator.

## Binary and source

- Original binary used by the first comparison: SHA-256
  `5e352e344e9cfb74d31f9197b450adda4c3b0b6df1720d9ec2ac2b10ed190c08`.
- Source checkout for the fixture: `/tmp/lat-wi1897-prep-20260810`.
- Raw comparison directory:
  `/tmp/wi1897-threeway-20260810/vmovups/register-reference/vmovups`.

## Reproduction command

```text
python3 tests/integration/run-latx-avx-three-way.py --execute --manifest tests/integration/latx-avx-opt-only-manifest.json --build-script tests/integration/build-latx-avx-wi1897-xzy86.sh --latx /home/xzy/github/lat/build64-todesk/latx-x86_64 --remote-host xzy86 --ssh-config /home/xzy/.ssh/config --remote-dir-root /tmp/wi1897-threeway-20260810/vmovups/register-reference --output-dir /tmp/wi1897-threeway-20260810/vmovups/register-reference --mnemonic vmovups --case register-reference
```

The fixture executes XMM VEX.128 register aliases first, followed by YMM
register aliases and memory forms. Each saved register record is 32 bytes:
the low 128 bits followed by the YMM high 128-bit shadow.

## Observed result

- x86 stdout: 384 bytes, SHA-256
  `984e61f4d388dea1c6d95b2a3db8093f4c164e61165763a5c21089f425021bbb`.
- LSX stdout: 384 bytes, SHA-256
  `f3964befd1bd89e2b1f9f52af51a5e866d380a72f149f84809c69ed4e5c6461e`.
- x86 and LSX exit status: `0`.
- LSX GDB log: `lsx.gdb.log`; `option_enable_lasx` was written and read back
  as `0` at `translate_context_init`.
- LASX exited with `-11` and produced no stdout; this is recorded separately.

The first byte differences from `cmp -l` are:

```text
17 0 210
18 0 167
19 0 146
20 0 125
21 0 104
22 0 63
23 0 42
24 0 21
26 0 377
27 0 356
28 0 335
29 0 314
30 0 273
31 0 252
32 0 231
```

These bytes are the high 128 bits of the first XMM results. x86 clears them
for VEX.128; LSX retained the destination's old high half. Later YMM records
matched. The common-path fix is to dispatch the LSX move by the decoded 128/256
bit operand width, so a 128-bit move always calls `clear_ymm_high128_shadow()`
and a 256-bit move copies the source high shadow. LASX code remains unchanged.
