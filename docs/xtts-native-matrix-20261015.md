# XTTS Native Matrix Validation, 2026-10-15

## Candidate

- Repository: `/home/xzy86/work/lat-no-lbt-o1-performance-20260919`
- Branch: `codex/no-lbt-recovery-20260919`, candidate after `103c2c4127`.
- Build host: `yuerengan@192.168.8.39:12581`.
- Board: `root@192.168.8.200`, idle before testing.
- Binary: `/root/xzy/aikit-xtts-hot-20261015/latx-x86_64-matrix-lsx2`.
- Runtime: `/root/xzy/aikit-board200-20261015/x86-test-root`.
- Evidence: `/root/xzy/aikit-xtts-hot-20261015/validate-lsx2-204204`.

The candidate replaces four complete SDK matrix functions, with original
instruction bytes checked only at matching translation entries. LA LSX loops
preserve integer lane overflow and separate floating-point rounding. All native
hotspot environment switches are read at startup. Old libmsc module identity is
classified on mapping insertion, rather than at every TB translation.

The clock_nanosleep helper now returns the POSIX error code directly and copies
remaining time only for interrupted relative sleeps. This fixes independent
semantic errors; it does not establish the cause of the earlier shutdown hang.

## Validation

`tests/integration/run-xtts-native-validation.sh` refuses an existing result
directory and creates a new HOME. No old AOT is copied. It records binary and
SDK hashes and parameters. Previous PCM is moved aside before testing, and each
new PCM is checked and moved to its own result file before the next run.

1. x86 scalar implementation versus original SDK: 40,000 comparisons passed.
2. LA vector versus scalar implementation: 40,000 comparisons passed at each
   of `-O1` and `-O2` on the compilation host.
3. AOT disabled on board: normal exit, newly generated PCM matches reference.
4. Empty-cache generation: no AOT loaded; successful generation observed.
5. Next invocation: nine AOT load successes and 284 recovery records observed.
6. All eleven invocations below exited normally and generated matching PCM.

Reference PCM SHA256:
`d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.

| Phase | Wall Seconds |
| --- | ---: |
| AOT disabled | 15.175 |
| Empty-cache generation | 17.375 |
| First load, AOT diagnostics enabled | 16.894 |
| Warmup 1 | 12.401 |
| Warmup 2 | 10.440 |
| Warmup 3 | 7.152 |
| Measured 1 | 6.898 |
| Measured 2 | 6.975 |
| Measured 3 | 6.940 |
| Measured 4 | 6.970 |
| Measured 5 | 6.840 |

Measured mean: **6.925s**; median: **6.940s**. Diagnostics are disabled for all
five measured runs. Loading some AOT is not itself evidence that every TB is
recovered or that runtime has stabilized. All warmup times are retained above.

## Remaining Limits

- The five-second objective is not met.
- Coverage is the supplied SDK test and randomized valid matrix inputs, not
  arbitrary x86 programs, exceptional pointer faults or all floating-point modes.
- Earlier scalar matrix candidate hung once during shutdown. The cause has not
  been established; the eleven-run result is non-reproduction, not a proven fix.

## Six-Function Candidate Saved for Review

The next candidate also replaces the complete
`ivMatrixCalcMul_AddBias16_In16_W16_Out16` and
`ivMatrixAdd_AddBias16_In16_W16_Out16_ColMajor` functions. The former truncates
to 16 bits rather than saturating; the latter preserves scratch-buffer writes.
Both use LSX in their integer loops. Entry byte signatures are checked before
substitution. `LATX_XTTS_MATRIX=1` is required; the new substitutions are not
enabled by default. This candidate does not include a native sinf_c replacement.

- Original SDK differential test: 60,000 comparisons passed.
- LA vector/scalar differential test at `-O1`: 60,000 comparisons passed.
- Binary: `/root/xzy/aikit-xtts-hot-20261015/latx-x86_64-matrix-col`.
- Evidence: `/root/xzy/aikit-xtts-hot-20261015/validate-col-205413`.
- Fresh JIT, empty-cache generation, new-cache loading and eight later runs:
  all eleven exited normally with newly generated matching PCM.

| Phase | Wall Seconds |
| --- | ---: |
| AOT disabled | 16.633 |
| Empty-cache generation | 15.244 |
| First load, AOT diagnostics enabled | 16.047 |
| Warmup 1 | 15.807 |
| Warmup 2 | 7.888 |
| Warmup 3 | 6.767 |
| Measured 1 | 13.884 |
| Measured 2 | 8.993 |
| Measured 3 | 8.754 |
| Measured 4 | 6.659 |
| Measured 5 | 6.615 |

Measured mean: **8.981s**; median: **8.754s**. Measured user CPU times were
6.745, 6.777, 6.802, 6.903 and 6.746 seconds. The large wall-time variation
has not been explained. The last two measurements alone do not establish an
end-to-end speedup over the four-function candidate. This is a saved opt-in
candidate, not evidence of meeting the five-second objective or resolving the
earlier shutdown hang.
