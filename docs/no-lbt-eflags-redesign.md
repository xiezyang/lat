# no-LBT EFLAGS Redesign

## Baseline and Isolation

Base: origin/no-lbt-x86_64-jit, 837a21cbe6.
Updated from e58ac5dec3 by fast-forward on 2026-09-24. The new upstream commit
adds a no-LBT FCVT fallback and corresponding AOT support. Earlier performance
measurements of e58ac5dec3 do not constitute validation of this updated base.
Branch: codex/no-lbt-eflags-redesign-20260924.
Repository: /home/xzy86/work/lat-no-lbt-eflags-redesign-20260924.
This is an independent clone, not a shared-metadata worktree.

Do not modify /home/xzy86/work/lat-no-lbt-o1-performance-20260919.
Its state is recorded at /home/xzy86/work/lat-eflags-state-20260924.
Do not import its uncommitted patches as part of the baseline.

## Implementation Order

1. Capture an unchanged baseline and flag correctness tests. Keep O1 flag
   reduction and the existing no-LBT pattern restrictions enabled.
2. Combine arithmetic-result and flag generation for register ADD/SUB first.
   Reuse the result without losing old operands needed by CF/OF/AF. Preserve
   partial-register semantics and the existing memory-fault ordering.
3. Separate flags consumed by fused Jcc from flags needed after either branch.
   Recompute flag demand after fusion; do not discard flags live at an exit.
4. Extend direct condition generation to selected SETcc/CMOVcc patterns.
   Rewrite software emission rather than enable LBT-specific patch paths.
5. Evaluate deferred flag computation within a translation block. Keep the
   existing packed flags at externally visible exits until signal recovery,
   AOT relocation and TU unlink behavior have dedicated validation.

Each stage needs an otherwise identical baseline, a correctness result and
generated-code counts before any timing claim. Do not interpret historical
producer-category counts as dynamic execution frequencies.

## Validation Conditions

- Local host: x86-64; use native guest reference and QEMU LoongArch for initial
  checks where appropriate, not as board-performance evidence.
- Build machine: yuerengan@192.168.8.23:12581, formerly 211.
- Board: root@192.168.8.200. O1, static, no KZT, no LBT x86 instructions.
- Keep AOT enabled and use independent caches for baseline and candidates.
- Leave LATX_SOFTFPU unset. The upstream baseline internally defaults to 2;
  record this behavior rather than silently changing it.
- In this baseline, explicitly setting LATX_AOT=1 can disable AOT because its
  argument handler checks option_softfpu. Use default-enabled AOT and verify
  actual cache generation/loading. Keep diagnostics out of timed runs.
- Verify 8/16/32/64-bit arithmetic, carry input, partial flag updates, branches,
  signal/exception recovery where affected, and cold/warm AOT correctness.
- XFYun reference WAV SHA256:
  e0f9aeaf8619a87fa510ba8891138aa0bc19e1dd1d2d10c72b9c428cdb21348a.

## Stage 1 Results

Based on 71b7f9f3b8 (explicit AOT/softfpu decoupling). Register ADD/SUB with
live flags now computes the arithmetic result once, derives flags before
overwriting the original operands, and copies the result to the destination.
The raw result is preserved for existing upper-bit/extension bookkeeping.
Memory and LOCK destinations retain the original path. Flag reduction and
pattern matching are unchanged.

Source sequence accounting: for 8/16/32-bit operations this removes two input
truncations; the second arithmetic instruction is replaced by a result copy.
For 64-bit operands the instruction count is unchanged at this stage. Whole
program generated-code counts have not yet been measured.

Compiled O1/static/no-KZT on 23. The new no-lbt-addsub-results.c test checks
160000 operations across four widths, boundary values and deterministic random
inputs, including result upper bits and all six defined arithmetic flags.
Native x86-64, 23 and 200 produce 2ed669d9e8b86198. The existing comprehensive
flag differential guest on 23 still produces SHA256
13d277b760a586ce605fdf2280e1d1e24b22f68d5569f0965759c51da7fe48a6.
Local QEMU did not produce valid output for this build; that run is not a pass.

Board 200 artifacts: /root/xzy/eflags-stage1-20260924 (the /tmp filesystem was
full). AOT enabled explicitly, LATX_SOFTFPU unset. All six XFYun runs generated
the reference WAV. The final three runs were 4.60, 5.02, 4.59 seconds. These are
smoke timings, not an interleaved performance comparison; no speedup claimed.
The cold/early runs were 10.34, 13.33, 7.65 seconds and are not warm timings.

Next: instrument comparable emitted-code counts and remove condition-only
flag demand after CMP/TEST fusion, preserving flags required by successor
blocks and recovery paths. Stage 1 remains an uncommitted isolated change.

## Stage 2 Candidate: Fused Terminal Branch Flag Demand

In ir1_optimization_over_tb, after recognizing a no-LBT CMP_JCC or TEST_JCC
pair at the end of the block, intersect the producer's flag definition mask
with the existing TU successor eflag_out mask. This excludes demand introduced
only by the replaced Jcc while retaining demand for the same bits from either
successor. Unknown successors remain conservative. This requires flag
reduction and instruction patterns; neither optimization is disabled.

The no-lbt-fused-flags.S guest checks taken CMP with subsequent CF consumption
by SETB/ADC, fallthrough TEST with subsequent ZF consumption, and dead-flag
successors, over 10000 iterations. Native x86, 23 and 200 return zero. Existing
ADD/SUB and comprehensive flag differential outputs remain unchanged on 23.

Board candidate: /root/xzy/eflags-stage1-20260924/lat-eflags-fusion-20260924.
Fresh AOT cache and logs: fusion-perf under that directory. Six XFYun outputs
match the reference WAV. Elapsed seconds: 10.53, 13.50, 7.92, 5.39, 4.73, 4.41.
These are correctness/warmup runs, not proof of speedup. Candidate remains
uncommitted. Actual fusion-elimination hit counts, emitted instruction savings,
AOT reload-specific coverage and interleaved timing remain to be measured.

## Restored Local Flag Optimizations

Migrated only flag-lbt.c and the local flag-generation changes from the frozen
experiment, retaining stages 1 and 2. Restored direct single-bit insertion,
constant-zero clearing, cheaper ZF/SF generation, redundant AF masking removal,
direct CF/OF Boolean insertion, logical CF/OF clearing and shared MUL/IMUL
overflow calculation. No unrelated AOT or floating-point patches were copied.

Build and all three flag tests passed on 23. ADD/SUB and fused-branch tests
passed on 200. XFYun with LATX_AOT=1 and LATX_SOFTFPU unset produced the expected
WAV in all six runs: 9.94, 12.72, 7.28, 5.28, 4.15, 4.04 seconds. These were
successive warmup/smoke runs, not an interleaved speedup measurement. Historical
13% producer and 8.56% total instruction reductions have not been remeasured
on this combined candidate.

Board binary: /root/xzy/eflags-stage1-20260924/lat-eflags-local-restored-20260924.
Logs and separate cache: restored-perf under the same directory.
Changes remain uncommitted; the frozen source directory was only read.
