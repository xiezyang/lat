# XTTS Guest Libc Fill Loop

## Change

`LATX_XTTS_MEMSET=1` enables a direct IR2 replacement for the supplied
runtime's libc loop at guest PC `0x55090df340`. It is disabled by default.
The fixed entry and 24-byte instruction hash must match. This loop writes
XMM0 four times, advances RDX by 64 and repeats while RDX is below RDI.
The replacement retains those stores, uses a native unsigned branch and
computes all six arithmetic flags only after the final iteration. It then
dispatches to the original tail. No host memset, wider access, skipped
work or approximate floating-point operation is involved.

## Correctness

Build host: `yuerengan@192.168.8.39:12581`.
Board: `root@192.168.8.200`; tests ran sequentially on an otherwise idle board.

`test-xtts-memset-guest.c` executes the real libc bytes with a return inserted
at the original tail, outside the hashed replacement. It tests 2,000
randomized lengths and final-bound offsets, nonzero XMM patterns, a prefix
canary and an inaccessible page after the permitted stores. Captured memory,
RDX/RDI, XMM0 and all arithmetic flags match native x86 with the replacement
both disabled and enabled. SHA256:
`f25fce81cd5059355bb620168983c22a8d4b7b4b84cf225169df4f1b5772f7a9`.
Translation diagnostics confirmed the entry was used.

Board binary `/root/xzy/aikit-xtts-hot-20261015/latx-x86_64-memset`, SHA256:
`cbc92b673851b1c812ee017f88c47d2e5340e65139afb9e8a1729d70ead5b2df`.

The first full validation, `validate-memset-003540`, passed AOT-disabled
execution (9.510 s), empty-cache generation (9.778 s), loading its own new
cache (10.938 s) and warm1 (5.723 s). Warm2 timed out after 36.043 seconds
with exit code 124 during initialization, before synthesis. This failure
is not excluded as measurement noise and its cause is unresolved. Three
subsequent runs using that cache completed in 4.456, 4.465 and 4.665 seconds,
with matching PCM. These retries do not explain or invalidate the timeout.

A second empty-cache validation passed all phases: JIT 9.460 s, generation
10.020 s, first load 10.728 s, warmups 5.837, 5.600 and 4.533 s. Five
measured runs were 4.503, 4.439, 4.439, 4.565 and 4.505 s: mean 4.490 s,
median 4.503 s. Every PCM check matched
`d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.

All performance measurements use the same isolated `runtime-dns-lan`,
LBT_X86=0, SOFTFPU unset and prior XTTS optimizations including JSON enabled.
They do not establish first-run sub-five-second performance, reliability of
the original DNS settings, or the cause of the initialization timeout.
Exceptional fault recovery inside fused loops is not exhaustively tested.

Evidence for the second validation:
`/root/xzy/aikit-xtts-hot-20261015/validate-memset-second-003756`.
Twenty further complete-process runs took 4.442, 4.476, 4.441, 4.453,
4.465, 4.468, 4.440, 4.539, 4.486, 4.506, 4.460, 4.449, 4.526,
4.426, 4.477, 4.456, 4.461, 4.442, 4.426 and 4.470 seconds. Mean
4.4655 seconds, median 4.4605 seconds, range 4.426..4.539 seconds;
all exited successfully and every PCM matched. Relative to the JSON-only
twenty-run mean of 4.5385 seconds, this is 0.0730 seconds (1.6%) less.
The tests are sequential sets, not randomized interleaving. The earlier
timeout remains unexplained despite these subsequent successful runs;
do not claim universal stability or enable this option by default.

User-cycle profiling at 999 Hz attributed 1.2602% of all counted periods
to `x86_55090df340` before the fill-loop replacement (`perf-json-v3.data`)
and 0.1649% after (`perf-memset.data`). The profiled run's new PCM matched.
These single-profile proportions support a local CPU reduction, not an
exact isolated wall-time attribution.

An additional twelve executions used the first validation's own cache,
with a watchdog configured to capture per-thread wait channels, kernel
stacks, mappings and an attached strace if execution exceeded eight seconds.
All twelve completed before that threshold with matching PCM. Evidence:
`/root/xzy/aikit-xtts-hot-20261015/watch-memset-004250`.
The timeout was not reproduced; no captured blocked-thread state exists,
so its cause and relationship to the fill-loop change remain unproven.
