# WI-1941 PRD: unblock WI-1940 compilation

## Goal

Remove only the compile errors found by the WI-1940 serial build in the
existing WI-1915 VPMOVX LSX implementation.

## Scope

- Make the `latx_vpmovx_extend_lsx_fn` type match the `IR2_INST *` return type
  of `la_vexth_hu_bu`, `la_vexth_h_b`, `la_vexth_wu_hu`, `la_vexth_w_h`,
  `la_vexth_du_wu`, and `la_vexth_d_w` at `tr-avx.c:2767-2769`.
- Add explicit unreachable returns after the default assertions at the two
  VPMOV wrapper switch statements at lines 2876 and 2896.

## Boundary and acceptance

Do not change instruction semantics, operands, generated IR, LASX functions,
tests for other instructions, or compiler warning policy. The source checks
must pass and the clean `build64-todesk` serial target build must proceed past
the reported errors.
