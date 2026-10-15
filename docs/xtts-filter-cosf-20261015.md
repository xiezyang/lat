# XTTS Filter Loop, Vector Scaling And Cosine

## Changes

- Add opt-in `LATX_XTTS_FILTER=1` for the tested SDK loop at
  `0x55182bf8b0`. The direct IR2 loop retains the preceding stored value in
  a floating-point register, removes intermediate CMP flag calculations,
  and restores final equal-CMP flags at exit. Preserve separate subtract,
  multiply and add operations. Unaligned or invalid bounds use ordinary
  translation. Fixed PC and instruction hash restrict the replacement.
- Add opt-in `LATX_XTTS_COSF=1`. Combine this SDK's double-precision
  pi/2 addition, float conversion, and existing sinf polynomial into one
  direct IR2 entry. Preserve operation order and defined register outputs.
  Guarded exceptional inputs use the original translation.
- Scale four matrix outputs with LSX, with explicit range masks to retain
  the original INT32_MIN result on invalid float-to-integer conversions.
  Direct LA conversion alone failed differential testing and was corrected
  before board integration testing.
- Add an optional LAN-specific resolver configuration for an isolated
  guest runtime. It uses 192.168.8.1, 223.5.5.5 and 119.29.29.29, with
  timeout:1 and attempts:2. It is not installed automatically, does not
  change system DNS, and is not a translator code speedup.
- Record guest-runtime resolver and hosts files in future validation
  provenance output, making environment differences explicit.

## Validation

Build host: `yuerengan@192.168.8.39:12581`.
Board: `root@192.168.8.200`.
Evidence root: `/root/xzy/aikit-xtts-hot-20261015`.

- Filter: 10,000 randomized arrays of lengths 1..256, comparing memory,
  pointer advance, XMM outputs and arithmetic flags. Enabled, disabled,
  and native x86 outputs matched SHA256
  `9dc21a99ebb6f69de62801b17be6fbec05c1c3c4f551c5cb01c61a39aae3656e`.
  The first version failed XMM high-bit comparison and was fixed before
  full SDK testing; only `filter-v2` and later binaries contain the fix.
- The optional third argument to the filter test exercises byte offsets
  0..3. This crashed both an older `latx-x86_64-lanes` binary without the
  filter change and the candidate's fallback path. It is an unresolved
  generic unaligned translation issue, not a passing test.
- Matrices: O1 and O2 each passed 100,000 vector/scalar comparisons plus
  169 four-lane conversion cases using zero, subnormal, infinite, NaN and
  boundary scale values. All tests use default floating-point mode.
- Cosine: 20,000 random inputs matched native x86 byte-for-byte. Of the
  18 special inputs, LAT's enabled/disabled results matched each other;
  four bytes differed from native x86 due to existing NaN-sign behavior
  on the ordinary fallback path. Sinf was also checked enabled/disabled
  after sharing the implementation, with the same existing NaN difference.
- Each board candidate passed AOT-disabled execution, generation with an
  empty cache, loading its own newly generated AOT, and subsequent runs.
  All checked new PCM files matched SHA256
  `d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.

## Timings

All rows use the same SDK and full process execution, LBT_X86=0, AOT enabled,
SOFTFPU unset, prior XTTS switches enabled, and TB_HASH_BITS=15. Times below
are after three warmup runs, not initial translation or first cache load.

| Candidate | Runtime DNS | Evidence directory | Five-run mean | Median |
| --- | --- | --- | --- | --- |
| Filter v2 | Google-first bounded | validate-filter-v2-233313 | 4.724 s | 4.735 s |
| Filter v2 | LAN bounded | validate-filter-lan-233505 | 4.728 s | 4.732 s |
| Vector scaling | LAN bounded | validate-convert4-233836 | 4.725 s | 4.726 s |
| Cosine | LAN bounded | validate-cosf-234555 | 4.708 s | 4.708 s |

The LAN filter candidate's ten additional runs were 4.815, 4.701, 4.718,
4.741, 4.768, 4.668, 4.746, 4.783, 5.369 and 4.752 seconds. One exceeded
five seconds. Vector scaling has no demonstrated isolated wall-time gain;
the cosine improvement is small relative to run-to-run variation.

Final binary `latx-x86_64-cosf` SHA256:
`bc81fa0454b4fc66601208651c0cc161dc6bd811992ea44b2a0e383ca507e0bf`.
Five measured times: 4.697, 4.757, 4.718, 4.659 and 4.708 seconds.
Initial JIT, generation and first cache load took 9.579, 10.203 and 16.691
seconds respectively. The first-load time is not a steady-state result.

Twenty further executions using this binary and its own cache all passed
PCM verification: 4.681, 4.727, 4.670, 4.777, 4.730, 4.723, 4.709, 4.723,
4.662, 4.755, 4.685, 4.752, 4.741, 4.720, 4.735, 4.730, 4.671, 4.830,
4.720 and 4.741 seconds. Mean 4.7241 seconds, median 4.7250 seconds,
minimum 4.662 seconds, maximum 4.830 seconds; none exceeded five seconds.
The result establishes repeated warmed-cache performance for this isolated
runtime configuration, not a guarantee for the original resolver or first run.

The complete-process target is not proven for the original resolver settings;
the previous checkpoint documents original-runtime DNS waits. No authorization,
network, synthesis or exit tasks have been bypassed. Exceptional recovery and
all nondefault floating-point modes are not exhaustively validated.

The final diagnostic `wait-cosf-lan.strace` confirms the guest opened
`runtime-dns-lan/etc/resolv.conf`, sent both actual SDK domain lookups to
192.168.8.1, and received successful DNS replies after 0.028392 and 0.028554
seconds. There was no five-second DNS timeout in this trace, and the newly
written PCM matched. Tracing overhead makes its total time unsuitable as a
benchmark. Other futex waits remain, including normal worker synchronization;
their per-thread elapsed times must not be added to process wall time.

## Final Four-Row i8 Checkpoint

The i8-input/i8-weight kernel now processes four rows together, sharing each
eight-byte input load and sign extension. Each row retains its original eight
independently wrapping signed 16-bit accumulators. The matrix differential
test passes 111,000 comparisons at each of O1 and O2, including dot products
up to 4,096 columns, plus the 169 special scaling cases described above.

Final binary: `latx-x86_64-i8four`, SHA256
`fe73bc4d3d4c4debb4704e5bfacb4df664d1e41ad00f18db06bc791633f0b4bc`.
Evidence directory: `validate-i8four-235443`. AOT-disabled execution took
9.696 seconds, empty-cache generation 10.207 seconds, and the first load of
that binary's own cache 11.133 seconds. All new PCM checks passed. Three
warmups took 6.194, 5.751 and 4.701 seconds. The following five measured
runs were 4.681, 4.652, 4.714, 4.690 and 4.693 seconds: mean 4.686 seconds,
median 4.690 seconds.

Twenty additional executions used the same binary, its own cache and the
isolated LAN runtime: 4.667, 4.688, 4.698, 4.645, 4.698, 4.667, 4.713,
4.701, 4.681, 4.723, 4.628, 4.697, 4.703, 4.705, 4.709, 4.702, 4.637,
4.651, 4.646 and 4.700 seconds. Mean 4.6829 seconds, median 4.6975 seconds,
minimum 4.628 seconds, maximum 4.723 seconds; all PCM checks passed and
none exceeded five seconds. Compared with the preceding cosine checkpoint's
twenty-run mean, the difference is approximately 0.041 seconds (0.9%).
This is a small incremental gain, not proof of an isolated causal speedup.

A further sixteen-column i8 loop passed differential and fresh JIT/AOT
checks but had no measured advantage (five-run mean 4.687 seconds versus
4.686 seconds). It was removed; the final source uses eight columns per
four-row iteration. These results still do not establish sub-five-second
first execution or performance with the original runtime's resolver.
