# XTTS Follow-up Hotspots, 2026-10-16

## Scope And Changes

Parent: db9b479980fce19de95cb5fb820d1bf5a7f69299. This follow-up covers
all four proposed areas: matrix arithmetic, remaining hot loops, hotspot
entry/exit overhead, and SDK/syscall waiting. No authorization, network,
synthesis or cleanup operation is skipped.

* Unroll the four-row i16 dot-product loop from eight to sixteen elements,
  retaining eight/four-element tails and modulo-32-bit accumulation.
* Matrix helpers read only their argument GPR copies. RAX..RDI live in LA
  s0..s7 and survive the host C ABI, so omit their redundant restores and
  omit stores of non-inputs. Keep all R8..R15, XMM, MMX and flags preservation.
* Add LATX_XTTS_MPI_TRIM for the two leading-zero scans in
  mbedtls_mpi_cmp_mpi. Generate compact LA loops and final arithmetic flags,
  retaining stack writes and original RAX/RDX values. Default is on.
* Add LATX_XTTS_AES for the complete supplied engine's
  mbedtls_aesni_crypt_ecb function. Reuse existing LSX AES rounds, keep their
  nine tables in registers for all rounds, and save/restore low FP registers
  once per function, not per round. Preserve encryption/decryption, output,
  XMM1's final key and other register results. Round counts outside 2..14
  use ordinary translation. Default is on.
* Before checking /proc/self/task identity, reject non-directory stat
  results. For statx, retain the old identity check if STATX_TYPE is absent.
* Batch synthesized /proc/cpuinfo output per CPU without changing its bytes.
* Provide separate example patches for the SDK test: configurable public
  log level and callback-based completion waiting instead of 1ms polling.
  These are test-program changes, not LAT speedups or SDK-library patches.

## Evidence

Build: yuerengan@192.168.8.39:12581,
/home/yuerengan/xzy/xfyun-hot-tu-20260929, existing static O1/no-KZT config.
Board: root@192.168.8.200. Evidence root:
/root/xzy/aikit-xtts-hot-20261015. Runtime is the unchanged runtime-dns-lan.
SDK shared libraries are unchanged. LBT_X86=0, LATX_SOFTFPU unset,
TB_HASH_BITS=15, all prior hotspot switches and AOT_UNMAP=1 enabled.

Every candidate uses a new HOME for AOT generation. Detached writers finish
before the next execution. Repeated tests use only that exact binary's cache,
not a different candidate's cache. All complete syntheses below produced PCM
SHA256 d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac.

Final LAT binary: latx-x86_64-next5, SHA256
58c9db5202db541e8565e2def819cfcd873739b97b63614b19f936be9fe3646d.

* Matrix vector/scalar: 111,000 comparisons each at O1 and O2 on host 39.
* Seven matrix entries: baseline/candidate full GPR, XMM, MMX, arithmetic
  flags and output-memory captures match (final-unit-next5).
* MPI: 20,000 original-x86 versus ordinary/replacement comparisons match,
  including all-zero values, unequal sizes and differing signs.
* AES: 10,000 comparisons with both modes, round counts 2..14, unaligned
  keys/input/output and in-place output match native x86. The existing
  LATX_VPAES=1 ordinary path also passes (final-unit-next5).
* stat/fstat/fstatat/statx, including STATX_NLINK-only requests: one and two
  guest threads produce expected task-directory counts; regular files match.
* Synthesized cpuinfo matches baseline byte-for-byte; output writes fall
  from 20 to two on the two-core board (cpuinfo-next5).

## Performance

Same-session baseline: repeat-baseline-114234, 20 runs,
mean 3.0167s, median 3.0015s, range 2.979..3.265s.

The next2 checkpoint (before resident AES tables/cpuinfo batching) passed
fresh JIT/generation/load at 9.331/9.786/2.991s. Its twenty further runs in
repeat-next2-114036 had mean 2.9369s, median 2.9380s, range 2.876..2.975s.
This is 2.6% lower mean and 2.1% lower median than the baseline above.

Same next2 binary with MPI_TRIM/AES both disabled used its own empty HOME
(validate-next2-loops-off-114543): JIT/generation/load 9.455/9.902/3.105s,
five subsequent runs mean 2.992s, median 2.984s. This is a combined control,
not separate attribution to either loop.

Final next5 fresh evidence: validate-next5-115602. AOT-disabled 9.250s,
empty-cache run 10.027s, extra writer wait 12s, first load 3.067s. Five runs
mean 2.951s, median 2.944s. Twenty further runs (repeat-next5-115710) had
mean 3.1090s, median 2.9560s, range 2.898..5.650s. The 5.650s and 3.363s
runs are NOT discarded. Thus final typical time improved modestly, but its
unconditional mean did NOT improve in this set. CPU time does not show the
same growth in the slow wall-time runs; the exact cause remains unproved.

A later sequential baseline/candidate repetition, both using the unmodified
SDK test, shows the environment changed during measurement:

| Twenty Runs | Mean | Median | Range |
| --- | ---: | ---: | ---: |
| repeat-baseline-final-120643 | 4.1659s | 4.0670s | 3.179..5.355s |
| repeat-next5-final-120823 | 3.6378s | 3.6980s | 2.977..4.555s |

All forty PCM files matched. Baseline now also has long runs, so the slow
times are not specific to the candidate. Logs show network failures in
some runs and continued SDK network-thread activity after AEE_End in others.
No competing benchmark was present. These nonstationary sequential sets
do NOT establish a 12.7% translator speedup. The reproducible claim is the
code/operation reduction and modest roughly-2% median improvement in the
earlier stable sets, not a guaranteed three-second upper bound.

Dot-only benchmark dot-unroll-bench: 100,000 four-row products per size,
three repetitions. For count 128..2048, unrolling reduces time about 2..7%;
count 64 is about 2% slower. Result hashes match for all sizes.

### Isolated AES And MPI Measurements

Final next5 binary, board 200, AOT disabled for both sides, three sequential
runs per setting. These are fixed-input function tests, not complete SDK
latency measurements. Translation/setup/output costs are included equally.
LATX_BENCH_REPEAT repeats each input without changing the captured result;
the AES test restores the original input before repeating an in-place call.

| Test | Calls Per Run | Disabled Times | Enabled Times | Mean Reduction |
| --- | ---: | --- | --- | ---: |
| AES function | 1,000,000 | 14.535, 14.527, 14.520s | 2.193, 2.192, 2.194s | 84.9% |
| MPI comparison | 60,000 | 2.610, 2.611, 2.610s | 2.068, 2.070, 2.068s | 20.7% |

Each enabled/disabled binary result stream matched byte-for-byte, including
captured registers and defined flags. Evidence directories:
`isolated-hot-bench-121541` (AES), `isolated-mpi-bench-121811` (MPI).
The first MPI benchmark used a dynamically linked test whose mappings
conflicted with the fixed library address; it exited 4 before running the
test and is not performance evidence. The static MPI test at repeat 100
exceeded the 35-second timeout with optimization off. That aborted attempt
is preserved separately; both completed sides above use repeat 3. AES uses
repeat 100. Neither microbenchmark percentage predicts the whole SDK gain.

## SDK Logging And Waiting

Public LOG_LVL_OFF=100 experiment, using an independently compiled SDK test
and fresh AOT (validate-log-off-112943): five-run mean 2.971s, median 2.968s,
versus original test mean 2.958s, median 2.961s at the next1 checkpoint.
No established total-time benefit; keep the original ERROR default.
log-level-{4,100}-next1-valid.strace show localtime path records 23 -> 11,
while futex starts remain 1199/1158 and ppoll 45/44. Log level does not turn
off authorization/event reporting. No log-print bypass hook is retained.

The old trace had 142 /proc/self/task path records; the candidate has 46.
These are path occurrences including unfinished lines, not exact syscall
completion counts. Final three full traces next5-full-{1,2,3}.strace show
46 task-path records each, 23 localtime records, 1226/1268/1278 futex starts,
and 893/998/973 clock_nanosleep starts. They all pass PCM equality. Most
1ms sleeps are the SDK example's main-thread polling, not engine workers.
Network poll waits generally return events; there is no evidence here to
justify suppressing work or increasing all timeout intervals.

Example patches assume LF line endings. Normalize the supplied mixed-line-
ending aikit_test.cpp in a separate SDK copy before applying:

```sh
sed -i 's/\r$//' aikit_test.cpp
patch -p1 < /path/to/xtts-completion-wait.patch
# Optional, independent logging experiment:
patch -p1 < /path/to/xtts-log-level.patch
g++ -std=c++11 -pthread -o aikit_test aikit_test.cpp \
    -Iinclude -Llibs -laikit -Wl,-rpath=libs
```

Completion waiting uses a condition variable with a mutex-protected
predicate. Completion before the wait and spurious wakeups are handled;
the existing timeout/error policy is not changed. No new thread is created.
This is an experimental patch, NOT the recommended/default benchmark.
validate-event-wait-120223 passed JIT/generation/own-cache load at
9.467/9.834/3.337s and five runs at mean 2.911s, median 2.899s. However,
repeat-event-wait-120331's twenty runs had mean 3.2476s, median 3.2810s,
range 2.810..3.701s (all PCM matched). Reduced CPU time did not establish
lower full-process latency. Keep the original SDK test as the default.
event-wait-full.strace confirms zero clock_nanosleep starts and 758 futex
starts. That run logged an online-auth network failure and had no ppoll
starts, unlike the earlier successful-network traces. Its futex count is
therefore NOT a controlled measure of the wait patch's futex savings.

Three further full traces, `cleanup-trace-121837/{1,2,3}.strace`, all produced
the expected PCM. Exec-to-exit_group spans were 4.543, 5.019 and 4.366s;
strace overhead means these are NOT uninstrumented benchmark times.
From the SDK's AEE_End log timestamps to exit_group, the spans were about
0.448, 0.468 and 0.267s. Network ppoll waits of roughly 2.2s overlapped
synthesis and returned events, so they cannot simply be added to latency
or removed. One main-thread connectivity pselect6 timed out after 0.200s.
These traces did not reproduce the earlier extra one-to-two-second cleanup
delay; its precise cause remains unproved. No authorization, network event,
thread job or shutdown step was suppressed to improve a measured time.

## Four-Area Completion Audit

| Requested Area | Implemented Or Tested | Evidence And Decision |
| --- | --- | --- |
| Matrix arithmetic | Sixteen-element four-row dot loop with tails | 111,000 scalar/vector cases at both O1/O2; isolated size-dependent gain, not a universal speedup |
| Remaining hot functions | Full AES function and MPI trim scans | Differential register/memory tests and isolated timings above; SDK fresh JIT/AOT checks pass |
| Entry/exit overhead | Remove redundant matrix GPR saves/restores | All seven entry captures match baseline including GPR/XMM/MMX/flags/output |
| SDK/syscalls/waiting | Directory-type stat filter, batched cpuinfo; logging and completion-wait experiments | stat variants pass, cpuinfo bytes unchanged and writes 20 to 2; no proven full-process benefit from SDK experiments, so retain original SDK test/libraries |

The four proposed areas have now been implemented or evaluated, including
board validation. This does not establish that all remaining performance
potential is exhausted, a two-second target is reached, or slow network
rounds are fixed. SDK patches remain independent experiments, not the
recommended default. Production release still requires broader regression
coverage described below.

## Duplicate mbedTLS Copies And SHA-256

The supplied SDK contains two mbedTLS copies. The engine and `libaikit.so`
versions of `mbedtls_aesni_crypt_ecb` are both 0x86 bytes with identical
bytes. The two `mpi_mul_hlp` copies are both 0x41a bytes with identical
bytes. Existing hotspot switches now recognize both addresses:

* `0x55182d0600` and `0x55084b1fb1` use the resident-table LSX AES path.
* `0x550846ae71` and `0x55182e7faa` use the native MPI multiply loop.

The AES differential test covers both copies, 10,000 cases, encryption,
decryption, rounds 2..14, unaligned input, and in-place output. The MPI
multiply test covers both copies, 20,000 cases, zero length, carry
propagation, and output memory. All copied-function outputs match the
ordinary translated code.

Isolated AES timing used 1,000,000 calls with AOT disabled. The engine copy
took 14.535/14.527/14.520s with the switch off and 2.193/2.192/2.194s with it
on. The MPI multiply test used 60,000 calls per run and took
2.610/2.611/2.610s off versus 2.068/2.070/2.068s on. These are function-level
measurements, not whole-process latency reductions.

Three additional MPI helpers were implemented with a host-side fast path:
`mbedtls_mpi_bitlen` at `0x55084690d5`, `mbedtls_mpi_shift_r` at
`0x550846a11a`, and `mbedtls_mpi_cmp_abs` at `0x550846a2f6`. Each passed
20,000 randomized comparisons. The fast path is behind
`LATX_XTTS_MPI_AUX=1`. In a complete AOT-disabled run, the control and
auxiliary runs used 11.064s and 11.117s wall time respectively, with nearly
identical user CPU time. No whole-process gain was established, so the
option remains off by default.

The SHA-256 compression implementation was removed after measurement. It
passed 20,000 randomized contexts for both copies, but an AOT-disabled
complete run exceeded 45 seconds versus about 9--11 seconds without it.
Repeated 64-byte calls made helper exit/relinking overhead dominate the
compression work. No SHA-256 helper remains in the source.

`mbedtls_mpi_copy` and `mbedtls_mpi_shift_l` were not enabled. Their target
capacity may require the SDK's own `mbedtls_mpi_grow` allocator. An in-TB
fallback after a native precheck was tested and found not safe: the original
translation resumed with damaged state. Keeping these paths disabled avoids
changing allocation behavior.

The final duplicate-copy candidate passed fresh JIT, empty-cache generation,
load of its own cache, and all PCM checks. The earlier sequential comparison
used all enabled runs first and then all disabled runs; network outliers in
that order are not a valid regression result. A later test alternated order
in eight pairs while switching between the two independently generated AOT
directories. Enabled mean/median wall time was 3.8533s/3.5995s versus
4.4091s/4.2430s disabled, and enabled was faster in seven of eight pairs.
Mean user CPU was 3.4366s enabled versus 3.5571s disabled. Network outliers
remain in both groups, so this is evidence against the earlier apparent
regression rather than a precise speedup percentage.

## Current Individual TB Profile

Three additional `next9` runs used `perf record -F 997 -e cycles:u` with
`LATX_PERF_MAP=1`. There were 8,747 JIT samples across 2,384 TBs. Only three
individual TBs reached 1% of JIT samples:

| Guest TB | Function | JIT share | Total share | Dedicated optimization |
| --- | --- | ---: | ---: | --- |
| `0x551818ae20` | `ivMath_cosf` | 1.475% | 0.85% | Yes, native cosine |
| `0x551818a770` | `sinf_c` | 1.418% | 0.82% | Yes, native sine |
| `0x5518165f3a` | `ReadResCB` | 1.017% | 0.59% | No |

`ReadResCB` is a 0x39-byte SDK callback. It computes `src + offset`, calls the
guest `memcpy`, and returns 1. It is a candidate for a direct native copy
path, but its complete removal has a whole-process upper bound of only about
0.6% of samples because it accounts for 89 of 15,114 total samples. The copy
work itself would remain, so the practical gain is smaller. No other
individual TB reached 1%; the next addresses are in `mbedtls_mpi_shift_r`,
FFT, SHA-256 and filter code at roughly 0.6--0.8% of JIT samples each.

## Limits

These remain opt-in, version/address-specific substitutions. Hash checks
occur only at matching translation entries, not every executed TB. Coverage
does not prove arbitrary fault/signal reconstruction in fused loops, all FP
modes, concurrent sessions, different voices or different SDK versions.
Current AOT cache options must match those used to generate that cache;
these tests deliberately use separate caches for changed code/options.
No safe thread-count reduction or general futex-elimination claim is made
for the new SDK. The previously tested EnginePool change did not remove
threads and regressed its old measurements; it is not applied here.

## Default Enablement

The XTTS hotspot switches that were observed in the supplied AIKIT workload
are now enabled by default: matrix, decrypt, clock-nanosleep, hot-TU, sinf,
expf, powf, logf, filter, cosf, MPI, JSON, memset, accumulate, MPI trim and
AES. The optional MPI auxiliary fast path remains disabled because it did not
show a complete-process benefit. Every switch can still be explicitly
disabled with `LATX_XTTS_<name>=0`.

The engine-cache-before-unmap option (`LATX_XTTS_AOT_UNMAP`) is also enabled
by default because it is required for the tested warm AOT result.

On the LoongArch board the host-capability initialization already forces
`option_enable_lbt = 0`, and AOT load/generation are initialized to enabled.
The release was therefore validated with only a writable `HOME` and the
SDK's `LD_LIBRARY_PATH`; no `LBT_X86`, `LATX_AOT` or `LATX_XTTS_*` variable
was set. The first run took 9.982s and generated the cache. The following
five runs took 3.050, 3.098, 3.054, 3.136 and 3.044 seconds, and every PCM
matched.

An earlier packaging build had left `CODE_GEN_HTABLE_BITS` at 20 even though
the stable three-second configuration used `LATX_TB_HASH_BITS=15`. That
default difference reproduced a 0.15--0.25 second regression in the
pre-`AEE_End` phase. The source default is now 15, so the package no longer
needs `LATX_TB_HASH_BITS`. On the board, direct warm runs with the corrected
binary were 2.916, 2.913, 2.899, 3.583 and 2.907 seconds. With the normal
installed runtime, three package runs had pre-`AEE_End` phases of 2.694,
2.765 and 2.738 seconds. Their remaining 1.4-second wall increase was after
`AEE_End` in the SDK network path.
