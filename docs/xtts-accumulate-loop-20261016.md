# XTTS Ordered Float Accumulation Loop

## Change

`LATX_XTTS_ACCUMULATE=1` enables direct IR2 for the supplied SDK loop at
`0x55182bedb8`, guarded by its 69-byte hash. It is disabled by default.
The loop updates two float-array positions per iteration. The replacement
retains the original load/store order, separate single-precision multiply
and add, signed index conversions, final GPR/XMM values and arithmetic
flags. It removes intermediate integer flag calculations and inter-TB
transitions. The final comparison's flags are computed on exit. Source and
destination may overlap; no vector reordering or FMA is used.

## Differential Validation

`test-xtts-accumulate-guest.c` calls the actual SDK bytes with a return at
the original continuation, outside the hashed region. It records EAX, EDX,
ECX, arithmetic flags, XMM0/XMM1 and the complete backing array.

- 10,000 finite-input cases, lengths 1..256, disjoint and overlapping
  arrays: native x86 and LAT with replacement off/on match byte-for-byte.
  SHA256 `cb717b2829c9be46132ea2f191f12d0ddb675cc268250b844c88b746b4049d26`.
- 10,000 cases including arbitrary float bits: enabled and disabled LAT
  match byte-for-byte, SHA256
  `95f8276fa47f00bc6d8dbab5d0f9a7c3895e7bf62fae4dd91f5a9711dfdf6a43`.
  Ordinary LAT already differs from native x86 for some NaN payloads in
  these inputs; this is not claimed as native equivalence for all NaNs.
- The first candidate incorrectly encoded an indexed address scale as
  eight instead of four. Differential tests caught the mismatch; only
  `accumulate-v2` contains the corrected scale.

Build host: `yuerengan@192.168.8.39:12581`.
Board: `root@192.168.8.200`, sequential tests on an otherwise idle board.
Binary: `/root/xzy/aikit-xtts-hot-20261015/latx-x86_64-accumulate-v2`.
SHA256: `00a4ea7e72de9748c63ecb32c1cfb3070f94564f125ef1dd07c4965414896681`.
Evidence: `/root/xzy/aikit-xtts-hot-20261015/validate-accumulate-v2-005006`.

Fresh validation passed AOT-disabled execution (9.481 s), empty-cache
generation (9.934 s), loading its own cache (15.241 s), warmups (5.854,
5.492 and 4.416 s) and five measured runs (4.429, 4.559, 4.502, 4.430
and 4.503 s; mean 4.485 s, median 4.502 s). All new PCM files matched
`d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.
The first cache load was slower than preceding candidates; no isolated
cause has been established and it must not be hidden by the warmed times.

Subsequent inspection found that the validation script waits for the guest
process but not its detached AOT-generation workers. The JSON, fill-loop
and accumulation validation sequences above therefore do not establish
isolated first-load timing: a preceding AOT worker may still have been
running. Their PCM checks remain valid, but fresh-phase performance must
be remeasured after explicitly waiting for those workers to finish.

Measurements use the isolated LAN DNS runtime, LBT_X86=0, SOFTFPU unset,
all prior XTTS switches enabled and LATX_TB_HASH_BITS=15. Nondefault FP
modes, FP exception status, asynchronous signal recovery and invalid-memory
fault reconstruction inside fused loops are not exhaustively tested.
The preceding fill-loop checkpoint's one unexplained initialization timeout
also remains a limitation.

Twenty subsequent full executions took 4.423, 4.410, 4.411, 4.444, 4.406,
4.572, 4.417, 4.489, 4.462, 4.533, 4.403, 4.431, 4.476, 4.485,
4.414, 4.431, 4.426, 4.414, 4.415 and 4.410 seconds. Mean 4.4436
seconds, median 4.4245 seconds, range 4.403..4.572 seconds; all passed
PCM verification and none exceeded five seconds. Relative to the preceding
fill-loop checkpoint's 4.4655-second mean, the reduction is 0.0219 seconds
(0.49%), a small gain that remains sensitive to run-to-run variation.

User-cycle sampling at 999 Hz attributed 0.4474% of counted periods to
`x86_55182bedb8` before this change (`perf-memset.data`) and 0.0536% after
(`perf-accumulate-v2.data`). The profiled run's new PCM matched. These
single-run profiles support a local CPU reduction, not precise process
wall-time attribution.
