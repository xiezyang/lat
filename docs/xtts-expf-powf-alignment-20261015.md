# XTTS expf/powf And Alignment Checkpoint

## Saved Changes

- Enable compiler-supported `-mstrict-align` for LATX LoongArch C/C++ builds.
  This prevents GCC from merging small-field accesses into unaligned wider
  accesses. Save lazy-link instruction pairs with two aligned 32-bit reads.
- Process eight elements per iteration in the LSX i16-input/i8-weight matrix
  kernel, preserving the existing eight-element count requirement.
- Add opt-in direct IR2 replacements for this SDK's expf and powf functions:
  `LATX_XTTS_EXPF=1` and `LATX_XTTS_POWF=1`. Both default to disabled.
  Fixed entry addresses and instruction hashes restrict substitution to the
  tested functions. Inputs outside the guarded ranges use ordinary translation.
  Preserve the original floating-point operation order without fused operations.
- Add guest differential tests and an optional resolver configuration for an
  isolated guest runtime. The configuration is not installed automatically and
  does not change the board's system DNS settings.

## Existing Validation

Build host: `yuerengan@192.168.8.39:12581`.
Board: `root@192.168.8.200`.
Evidence root: `/root/xzy/aikit-xtts-hot-20261015`.
Final tested binary: `latx-x86_64-powf`, SHA256
`75d45aa3611bba9b5153f56da9b79dcf48a1ce3127fbcf9235178452e3f00157`.

- The IR2 alignment change from the preceding commit was subsequently tested
  on the board. Alignment faults fell from approximately 1,211,700 before that
  change to 66,209 after the additional strict-alignment and lazy-link changes.
  These measurements include application activity, not just LAT code.
- LA matrix scalar/vector comparisons passed 70,000 cases at O1 and O2.
- expf passed 20,000 randomized and 18 special-input guest differential cases.
  powf passed 20,000 randomized cases; an additional 36 special-input cases
  matched with the replacement enabled and disabled.
- Successive candidates passed AOT-disabled execution, generation with an empty
  cache, and loading their own newly generated AOT. Checked new PCM files had
  SHA256 `d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.
  No old version's AOT was copied in to validate new code.

With the original runtime, the final powf candidate's five measured runs were
4.848, 13.635, 4.880, 4.834 and 8.233 seconds: mean 7.286 seconds, median
4.880 seconds. Separate strace runs observed five-second DNS timeouts followed
by successful retries. This explains some variation, not every slow run.

With the isolated runtime's bounded DNS configuration, the final fresh-cache
validation's five measured runs were 4.862, 4.841, 4.884, 6.448 and 4.876
seconds: mean 5.182 seconds, median 4.876 seconds. Runtime DNS changes are
environment configuration, not LAT code speedups. Stable sub-five-second
execution has not been established.

## Remaining Limits

- One earlier strict-alignment candidate timed out after 35 seconds despite
  producing the expected PCM. Its shutdown problem is not proven resolved.
- Floating-point tests cover the supplied SDK and default FP mode, not every
  rounding mode, exception or asynchronous recovery case.
- The existing generic unaligned-f64 test timed out with both an older binary
  and this candidate; the unaligned-v128 test passed. Do not interpret SDK
  validation as passing all generic translator regressions.
- DNS retries and other occasional slow runs remain. This commit saves tested
  progress, not a fully validated release or a completed performance target.
