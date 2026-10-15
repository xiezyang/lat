# XTTS Logarithms And Four-Row Matrix Kernels

## Implementation

- `LATX_XTTS_LOGF=1` enables direct IR2 replacement of this SDK's
  `ivMath_logf` and `ivMath_log10f`. It is disabled by default. Entry addresses
  and function-byte hashes must match. Positive normal inputs use the rewritten
  operation sequence; all other inputs use ordinary translation. The constants,
  floating-point operation order, defined flags, GPR and XMM outputs, and the
  stack scratch write are retained. No host libm or fused operations are used.
- The i16-input/i16-weight matrix kernels process four rows together, loading
  input once rather than twice. Four-element column tails are retained. The
  32-bit-output kernel retains the two-row path for remaining rows.
- Matrix output scaling uses explicit separate LA multiply/add/multiply
  instructions instead of volatile intermediate stack writes.
- The i8-input/i8-weight kernel sums its wrapped signed 16-bit lanes in vector
  registers instead of storing them and loading each lane individually.

## Validation

Build host: `yuerengan@192.168.8.39:12581`.
Board: `root@192.168.8.200`; no concurrent application test was running.
Board evidence root: `/root/xzy/aikit-xtts-hot-20261015`.

- The logarithm guest test produced byte-identical captured register, flag,
  and XMM outputs to native x86 across 40,000 randomized inputs and 30 special
  inputs. Board diagnostics confirmed both replacements were generated.
  Output SHA256: `af0ba676dcbdff1c6f2ea650fcffe1673863ee6d57099d5552d7581b81118632`.
- The expanded LA vector/scalar matrix tests passed 100,000 comparisons at
  each of O1 and O2, including zero dot-product counts, four-element tails,
  and variable even row counts.
- Every candidate used the validation script: AOT disabled, empty-cache
  generation, loading its own cache, three warmups, and five measured runs.
  All checked new PCM files matched SHA256
  `d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.

| Candidate / evidence directory | Five-run mean | Median |
| --- | --- | --- |
| Four rows: `validate-four-230922` | 5.301 s | 4.860 s |
| Add logarithms: `validate-logf-231233` | 4.792 s | 4.798 s |
| Explicit FP scaling: `validate-matrix-fp-231423` | 4.806 s | 4.797 s |
| Vector lane sum: `validate-lanes-231748` | 4.743 s | 4.728 s |

The explicit FP scaling change alone has no demonstrated wall-time speedup.
It removes stack traffic while preserving separate floating-point operations.
Measurements of these small changes remain sensitive to noise.

Final tested binary: `latx-x86_64-lanes`, SHA256
`ae35aadfbfafcda1b75dd65fffe9ae2e9e6e7bfea0139d66c70f0c267ff5348d`.
Its five measured runs were 4.794, 4.722, 4.715, 4.728 and 4.754 seconds.
Its AOT-disabled run took 9.583 seconds, initial generation 10.021 seconds,
and first load 11.159 seconds. The five-run result applies only after warmup,
not to first execution or initial cache loading.

## Environment And Limits

These runs used `runtime-dns-bounded`, an isolated copy of the original x86
runtime with the resolver configuration saved in
`tests/integration/xtts-runtime-resolv.conf`. No system DNS settings changed.
DNS changes are environment configuration and must not be counted as LAT
code-only improvements. All existing SDK tasks, including network and exit
cleanup, remain enabled.

Enable the prior XTTS HOT_TU, DECRYPT, CLOCK_NANOSLEEP, MATRIX, SINF, MPI,
EXPF and POWF switches as well as LOGF; set `LATX_TB_HASH_BITS=15`,
`LBT_X86=0`, and leave `LATX_SOFTFPU` unset. The new math replacement remains
specific to the supplied SDK and default FP-mode test coverage. The previous
checkpoint's exceptional recovery and occasional exit-wait limitations remain.

The explicit-FP candidate's subsequent ten runs averaged 4.891 seconds with
median 4.791 seconds, but two exceeded five seconds (5.193 and 5.463).
Do not claim every complete execution is below five seconds from the five-run
tables alone. Further repeated runs of the final vector-lane candidate are
stored alongside its validation results: 4.783, 4.727, 4.897, 4.785, 4.755,
4.798, 5.981, 4.762, 4.722 and 4.714 seconds; mean 4.892 seconds, median
4.773 seconds. All ten PCM checks passed. One complete execution exceeded
five seconds, so stable sub-five-second execution remains unproven.

## Original Runtime Cross-Check

The identical final binary was also validated from an empty cache with the
original `/root/xzy/aikit-board200-20261015/x86-test-root` runtime, without the
resolver override. Evidence: `validate-lanes-original-232201`. All eleven
new PCM checks passed. The five measured wall times were 4.757, 13.451,
8.337, 4.794 and 8.500 seconds: mean 7.968 seconds, median 8.337 seconds.
Their user CPU times stayed between 5.455 and 5.587 seconds.

For example, run2 logged successful AEE_End and offlineSave at 23:23:04,
but the network-related genCurrentTime messages continued at 23:23:11 and
23:23:12 before process exit. This corroborates a post-synthesis wait; these
timestamps alone do not identify every syscall causing it. Earlier strace
captured DNS timeout/retry behavior. The original-runtime results do not
meet the requested complete-process five-second target. Do not describe the
bounded-runtime timing as a code-only speedup or as fixing all exit waits.

A subsequent cycles profile (`perf-lanes-current.data`) attributed 3.30% of
user cycles to latx_xtts_matrix and 1.50% to xtts_dot_i16_four. The preceding
powf candidate's profile attributed 4.53% to latx_xtts_matrix and 2.18% to
xtts_dot_i16_pair. These single-profile proportions support less matrix CPU
work but do not establish an exact isolated wall-time speedup.

Final original-runtime trace: `wait-lanes-original.strace` (strace with
`-f -ttt -T -yy`), with a matching newly written PCM. Network thread 15228
experienced a 5.005102-second DNS poll timeout; later DNS requests to
1.1.1.1 took 0.305550 and 0.630969 seconds. Main thread 15218 eventually
waited 3.646308 seconds in futex during exit. Trace timings include tracing
overhead and are diagnostic, not benchmark results. No network tasks were
skipped to obtain these measurements.
