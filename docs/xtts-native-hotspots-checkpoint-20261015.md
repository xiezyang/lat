# XTTS Hotspot Checkpoint, 2026-10-15

## Changes

- Vectorize the existing decrypt helper with LSX while retaining the scalar
  behavior for forward-overlapping output and the original error codes.
- Add opt-in direct IR2 replacements for the supplied libaikit.so's
  `mpi_mul_hlp`, `mpi_sub_hlp`, `add32` and `sub32`. Enable them with
  `LATX_XTTS_MPI=1`; default remains disabled. Entry addresses and instruction
  hashes restrict substitution to the tested SDK.
- Add the seventh native matrix function, with 8-bit inputs and 16-bit weights,
  under the existing `LATX_XTTS_MATRIX` switch. Widen the 16-bit dot-product
  vector loop and cover its four-element tail.
- Correct the vector/scalar test's macro cleanup so it actually compares the
  two implementations, and include four-element tails in randomized inputs.
- Add `LATX_TB_HASH_BITS=12..20` to select the initial TB hash-table capacity.
  The default remains 20, automatic growth remains enabled, and flush uses
  the configured initial capacity.
- Align `IR2_OPND` to eight bytes. The previous four-byte alignment let GCC 8
  emit unaligned eight-byte operand copies at IR2 instruction offsets 12 and
  20. A board alignment-fault profile motivated this final change.

## Validation And Limits

Repository: `/home/xzy86/work/lat-no-lbt-o1-performance-20260919`.
Build host: `yuerengan@192.168.8.39:12581`.
Board: `root@192.168.8.200`.
Board evidence root: `/root/xzy/aikit-xtts-hot-20261015`.

Before the final IR2 alignment change, successive candidates passed the
validation script's AOT-disabled run, empty-cache generation, loading their
own newly generated AOT, and subsequent runs. All checked new PCM files matched
SHA256 `d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.
No other version's AOT was copied into these result directories.

- Decrypt: 12,000 randomized comparisons, overlap, errors and guard pages.
- Matrices: 70,000 SDK comparisons and LA scalar/vector comparisons at O1/O2.
- Each of the four integer replacements: 20,000 guest differential cases,
  including output memory, register outputs and arithmetic flags.
- Local save-time checks repeated the 12,000 decrypt comparisons against the
  original SDK and the 70,000 matrix comparisons on x86. The latter is not
  a substitute for the earlier LA vector tests.
- The complete current source, including the final alignment change, built
  successfully on host 39. The final alignment change has NOT yet been run
  on board 200 with fresh JIT/AOT or measured for fault reduction.

The last five measured board runs before the alignment change, in
`validate-word32-220531`, were 13.284, 5.957, 5.970, 13.397 and 6.034 seconds:
mean 8.928 seconds, median 6.034 seconds. DNS retries during SDK shutdown
were observed in a separate strace and are a source of wall-time variation.
These numbers do not establish a stable speedup or meeting the five-second
target. No system DNS settings were changed.

Coverage is the supplied SDK and differential inputs, not arbitrary x86
programs or complete exceptional-pointer and floating-point-mode behavior.
The alignment change and optional hash sizing still need broader regression
tests. This commit saves the current candidate, not a fully validated release.
