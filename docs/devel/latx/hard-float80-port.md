# Experimental Hardware Floating-Point Port

## Source And Scope

The port uses upstream master `5e52e497307d34c23479731095fc86f6074c3b3d`.
Its source is the `option_float80_128cal` branch ending at
`17e49b04b4c`, using the final functional changes after
`2290ea315ca2b6fa8471bc799da46747419bfca5` rather than merging its old
repository history. That starting commit reverts an older softfpu status-word
implementation; current upstream already has its own implementation and later
rounding and helper-state fixes, which this port preserves.

Included features:

- Experimental 80-bit arithmetic, conversion, movement, square-root and
  comparison instruction templates and translation paths.
- `LATX_FLOAT80`, `LATX_STACK` and `LATX_128CAL` options, disabled by default.
- Full-width x87 register transfers around helpers in hardware stack mode.
- Six scalar SSE add/subtract/multiply instructions preserving high XMM bits.
- A 64-bit `LATX_SOFTFPU_FAST` mask so bits 32 through 36 are not truncated.

Not imported: debug print functions, commented instrumentation, unused PC
tracking fields, instruction-enum reordering, global LASX disablement and
machine-specific static-link build-script changes. Existing upstream hardware
detection, helper-region planning and independent x87/SSE rounding remain.
Hardware stack mode uses single-instruction helper regions because ordinary
inline x87 operations must execute with hardware TOP mapping enabled.

The instruction encodings come from the source branch, not from a claim that
existing LoongArch LSX/LASX/LBT implementations support them. There is no
automatic detection of this experimental hardware extension. See
`docs/user/latx-environment.rst` for option values and requirements.

## Validation

On a LoongArch 3A6000 host, both x86_64 and i386 O1 translators built with
tests enabled, without KZT or AVX-specific build options:

```sh
mkdir build-float80-tests
cd build-float80-tests
../configure --target-list=x86_64-linux-user,i386-linux-user \
    --enable-latx --optimize-O1 --disable-docs --enable-tests \
    --extra-ldflags=-ldl --extra-cflags=-Wno-error=nonnull
python3 ../meson/meson.py configure -Dwerror=false
ninja -j6
cd ..
python3 meson/meson.py test -C build-float80-tests \
    --suite lat-pr-fast --print-errorlogs
python3 meson/meson.py test -C build-float80-tests \
    --suite latx-integration test-float-options-default-x86_64 \
    test-float-options-default-i386 --print-errorlogs
```

The configure probe for `clock_adjtime()` triggers a nonnull warning with the
host headers. An existing AOT test fixture also triggers an unused-function
warning. These build-directory overrides do not change the product scripts.

- All 53 `lat-pr-fast` tests passed, including template, code-generation guard
  and compiled 64-bit option-selection checks.
- Both new integration tests passed. They cover 18 guest/mode combinations
  with the new instructions disabled: softfpu modes 0/1/2, FAST values
  0/-1/0x100000000, x87 arithmetic/comparison/movement, and all six scalar SSE
  operations with high-XMM sentinels.
  They also check that incompatible hardware-stack options are rejected.
- A source-level regression check covers the integer-store helper fallback:
  it must pop the software stack before the helper epilogue restores hardware
  TOP mapping. This check does not execute the experimental instructions.
- The 23 existing affected floating-point integration tests were selected but
  skipped because Clang was not available to build their guest fixtures.
- `git diff --check` passed.

## Remaining Validation

Experimental instructions were not executed on the 3A6000 host. A compatible
hardware or FPGA target must verify the enabled FLOAT80/STACK/128CAL paths,
rounding and exceptional values, x87 stack transitions, helper calls, signals
and state save/restore. Instruction-template tests only confirm the imported
encodings and selection logic, not hardware semantics. AVX-enabled and KZT
build configurations and real applications have not been validated by this
port. The skipped integration tests must also be rerun with Clang/LLD or
verified guest fixtures. This branch is not a hardware acceptance result.
