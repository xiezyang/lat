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

## Pattern Expansion Scope (2026-09-26)

User decision: record defects also present in master, but do not fix them as
part of no-LBT instruction-pattern expansion. Validate no-LBT adaptations for
new regressions and distinguish inherited defects from adaptation failures.

Source inspection found missing signed division overflow checks in the
CQO + IDIV and CDQ + IDIV fused translators: minimum signed integer divided
by -1 must raise an x86 divide exception. Both implementations check a zero
divisor but do not explicitly check this overflow. The same code is present
in the local upstream master snapshot 97429a3, at tr-pattern.c:708 and :988;
the relevant division code traces back to cbdb01a35a0. This finding has not
yet been reproduced at runtime, and latest remote master has not been checked.
The current no-LBT pattern mask excludes these two fusions; individual
instruction translation remains available. Record this inherited limitation
when evaluating expanded coverage; do not claim full x86 correctness solely
from agreement with master.

## Pattern Expansion Candidate

The no-LBT option mask now includes all 26 existing pattern bits (0x3ffffff).
Added software paths for adjacent BT/SUB/SHR/AND branches and separated
CMP/TEST/BT branches. Separated integer producers materialize live flags at
the producer; no-LBT recovery no longer invokes raw LBT pattern assembly.
Adjacent floating comparisons materialize live flags before branching.
Enabled existing register-only division, CMP/SBB, NEG/CMOV and floating
SET/branch patterns. Zero-count SHR is deliberately not fused: it reads
the preceding ZF rather than the unshifted operand's zero status.

Memory operands remain excluded from integer/float comparison fusion.
Separated patterns only cross NOP, LEA and non-memory MOV/MOVZX/MOVSX/MOVSXD.
The four-store pattern retains its existing SMC eligibility checks.
XOR/DIV is enabled in the mask but its inherited is_contain_edx(opnd0)
check makes the intended EDX/RDX-zeroing pattern unreachable. Confirmed
in local master 97429a3; left unchanged per the user's scope decision.

23 build: /home/yuerengan/xzy/instpattern-expand-20260926/build/latx-x86_64.
Independent source copy was resynchronized from the local repository after
detecting diagnostics in the initially reused source. O1, static, debug.
Tests used LATX_HOST_HWCAP=0x30, LATX_AOT=1 and LATX_SOFTFPU unset.
Seven assembly guests (bt, and, sub-shr, separated, integer-pairs, float,
vector) passed native x86 and on 23. Existing EFLAGS differential comparison
passed 327680 defined-result/flag/condition cases on two runs. Initial cached
smoke results were not accepted as fresh-path evidence; corresponding BT,
AND, SUB/SHR and separated test cache files were moved into the test directory
and fresh translation dumps captured before repeat runs. AOT file loading
was observed with host strace. Vector store correctness passed with LATX_SMC=6,
but four-store fusion hits have not been established. Full pattern-hit,
signal/exception recovery coverage and board-200 XFYun regression/performance
remain outstanding. This is an uncommitted candidate, not a full correctness
certification of all 26 patterns or all operand forms.

Board 200 follow-up: all seven assembly guests returned zero. XFYun failed
with exit 133 in translate_sub_jcc at ra_free_temp(lhs): physical register
12 was no longer allocated (itemp_status 0xc) after translate_sub. This is
an adaptation regression; the candidate is NOT ready for use or submission.
The nested ordinary SUB translator and the outer fusion do not preserve the
expected temporary-register ownership. The precise inner release/clobber
site still needs investigation. Do not fix this merely by deleting the free:
the saved comparison operand may also have been overwritten.
Artifacts: /root/xzy/instpattern-expand-20260926/xfyun.{out,err,wav}.
Elapsed 12.83 seconds is a failed run, not a performance result; WAV is empty.

SUB/Jcc follow-up: keep saved comparison inputs in the existing a0/a1 pattern
registers instead of retaining two itemps across translate_sub. This avoids
pressure on the seven-register itemp pool and the observed ownership failure.
The exact inner allocation/release responsible has not been instrumented;
pool exhaustion remains the working explanation, not a proven trace.
Added an 8-bit SUB/JB test that reads all six flags with PUSHFQ and checks
partial-register preservation. Native x86 and freshly translated no-LBT on
23 pass. The 327680-case differential suite also passes after this change.

Board binary: /root/xzy/instpattern-expand-20260926/lat-pattern-fixed-20260926.
XFYun now succeeds with AOT=1 and LATX_SOFTFPU unset. First elapsed time 16.82s;
three subsequent runs 4.65, 4.76, 4.70s (median 4.70s). All four WAVs match
e0f9aeaf8619a87fa510ba8891138aa0bc19e1dd1d2d10c72b9c428cdb21348a.
There was no interleaved baseline comparison, so no speedup is claimed.
This supersedes the failure status above for XFYun; exhaustive exception
recovery and all-pattern hit coverage are still not established.

## Matched Build Timing (2026-09-26)

The earlier candidate used --enable-debug whereas the deployed bc86594b01
baseline did not. An isolated-cache comparison of those mismatched binaries
gave medians 3.80s vs 4.69s; it must not be attributed solely to pattern changes.
Rebuilt the candidate without --enable-debug using the baseline's configure
options (O1/static/no debug info). Each binary used its own HOME/cache and
five warmup runs, followed by five alternating timed runs on board 200.
LATX_AOT=1, LATX_SOFTFPU unset, identical input, all WAV checks passed.

| Round | Baseline bc86594b01 | Expanded patterns, release |
| --- | --- | --- |
| 1 | 3.75 | 3.89 |
| 2 | 3.74 | 3.62 |
| 3 | 3.80 | 4.06 |
| 4 | 3.79 | 3.79 |
| 5 | 3.88 | 3.92 |

Medians 3.79s and 3.89s (+2.64%); means 3.792s and 3.856s (+1.69%).
No measured speedup. Five samples do not establish a significant regression.
Artifacts: /root/xzy/instpattern-release-timing-20260926; candidate binary
/root/xzy/instpattern-expand-20260926/lat-pattern-release-20260926.
The old diagnostic instruction-count sample (829435 vs 824979 at 27600 TBs)
also compared the mismatched builds and is not a clean pattern-only count.

## Fused Demand and TU Link Correction

Extend reduce_fused_conditions to separated integer branches and adjacent /
separated floating branches. Remove the fused consumer's demand while keeping
the successor demand. Software integer producers still emit required flags
at the original producer location. Cross-TB fixed-point propagation is not
rerun after fusion, so predecessor demand can still be conservative.

Add soft_pattern_tu_branch to retain TU conditional branch relocation and
unlink fallback for no-LBT fused branches with both successors known. All
three eflags_target_arg positions stay invalid: only branches are relocated,
never software flag sequences. Constant branches without a final conditional
IR2 branch retain the ordinary exit path.

23 debug and release builds pass; seven pattern guests pass three runs from
new guest paths, and the 327680-case EFLAGS differential test passes.
Board 200 release comparison uses separate AOT caches, five warmups each,
LATX_AOT=1 and LATX_SOFTFPU unset. All WAV hashes match the reference.
Baseline bc86594b01 times: 3.92, 3.97, 3.89, 3.72, 3.81 seconds.
Corrected candidate: 3.58, 3.93, 3.70, 3.91, 3.69 seconds.
Medians: 3.89 vs 3.70 (-4.88%); means: 3.862 vs 3.762 (-2.59%).
Five samples only; no claim of a large or universally repeatable speedup.
Logs: /root/xzy/instpattern-tu-fixed-timing-20260926.
Binary: /root/xzy/instpattern-expand-20260926/lat-pattern-tu-fixed.
Full asynchronous exception / forced TU unlink coverage and matched generated
instruction counts remain outstanding. Changes are not committed or pushed.
