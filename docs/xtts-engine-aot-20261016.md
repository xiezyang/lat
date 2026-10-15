# XTTS Engine Cache And Mixed Exit Labels

## Changes

The guarded filter replacement emits an indirect fast-path exit and an
ordinary translated fallback in the same TB. During TU translation, that
fallback can acquire TU branches. `label_dispose()` previously resolved
only the indirect label, leaving the TU unlink-stub position as a label
number rather than a byte offset. TU compaction then moved the wrong byte
range. Loading the newly generated engine AOT reproduced a SIGSEGV, with
a recovered TB address ending in an unaligned byte offset. Resolving both
exit types made fresh engine-cache loading pass. Ordinary fallback direct
jump labels are also resolved when there is no TU jump.

Add opt-in `LATX_XTTS_AOT_UNMAP=1`, disabled by default. Before unmapping
the tested engine executable segment containing `0x551815c000`, check its
basename (`ebd1bade4_v1025_aee.so`) and full-segment coverage. If no AOT was
loaded for it, stop guest CPUs with the existing bounded exclusive wait,
recheck under mmap_lock, then fork the existing AOT writer before the
original unmap removes the engine from the segment table. That writer
generates only the engine library. The normal final writer still handles
the other libraries. Do not take mmap_lock while waiting for other CPUs.
Skip this snapshot when the engine already has loaded AOT.

No engine work, synthesis, authorization, network activity or cleanup is
removed. The SDK libraries and isolated LAN runtime were not changed.
Temporary signal/code-dump diagnostics and an ineffective lazy-link reset
experiment were removed. An experiment suppressing thread-exit AOT writers
was rejected and removed.

The validation script now waits for detached AOT writers belonging to the
same binary and HOME before starting the next run. It reads NUL-delimited
environment entries with Bash: the board's BusyBox grep does not support
`-z`. Earlier tests using grep -z did NOT reliably wait for workers. Preserve
the exact first-generation cache after all writers finish, before loads
can extend it. The previously reported 8--9 second first loads overlapped
unfinished cache generation and do not establish isolated loading costs.

## Board Validation

Source repository: `/home/xzy86/work/lat-no-lbt-o1-performance-20260919`.
Build host: `yuerengan@192.168.8.39:12581`, existing static O1/no-KZT build.
Board: `root@192.168.8.200`, sequential tests with no competing LAT process.
Binary: `/root/xzy/aikit-xtts-hot-20261015/latx-x86_64-unmap-v3`.
SHA256: `36d35365847be6b0c6b014d1094818d843628e416e5eadb59b2a5ba180f24a50`.

Settings: LBT_X86=0, LATX_SOFTFPU unset, LATX_AOT=1, TB_HASH_BITS=15,
all previous XTTS hotspot switches enabled, plus LATX_XTTS_AOT_UNMAP=1.
Runtime: `/root/xzy/aikit-xtts-hot-20261015/runtime-dns-lan`, unchanged.
The isolated resolver settings remain a prerequisite of these measurements;
they are not a translator speedup or a change to system DNS.

Evidence: `/root/xzy/aikit-xtts-hot-20261015/validate-unmap-v3-015226`.

| Phase | Full-process wall time |
| --- | --- |
| AOT disabled | 9.435 s |
| Empty-cache execution/generation | 10.224 s |
| Additional wait for detached generation | 12 s |
| First load of this binary's newly generated cache | 3.089 s |
| Three warmups | 3.020, 3.011, 3.009 s |
| Five runs | 3.004, 2.975, 2.985, 3.022, 2.964 s |
| Five-run mean / median | 2.990 / 2.985 s |

Repeated evidence:
`/root/xzy/aikit-xtts-hot-20261015/repeat-unmap-v3-015418`.
Three separate HOME directories each received only the preserved exact
first-generation cache from this binary. Their first process executions
took 2.990, 2.990 and 2.989 seconds. No other binary's cache was used.

Twenty further complete process executions took 3.049, 3.092, 2.973,
2.978, 2.985, 2.992, 3.003, 2.980, 2.987, 3.002, 2.992, 3.236,
3.029, 3.023, 2.997, 2.988, 2.993, 2.989, 3.004 and 2.977 seconds.
Mean 3.0134 s; median 2.9925 s; range 2.973..3.236 s. All passed.
Every phase and repeated execution produced new PCM matching SHA256
`d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.

The final binary also reran the isolated 10,000-case filter test with
AOT disabled and the filter switch off/on. Both byte streams matched
the existing native-reference capture, SHA256
`9dc21a99ebb6f69de62801b17be6fbec05c1c3c4f551c5cb01c61a39aae3656e`.
Evidence: `/root/xzy/aikit-xtts-hot-20261015/filter-unmap-v3-015616`.

Compared with the preceding checkpoint's twenty-run mean 4.4436 seconds,
the new mean is 1.4302 seconds (32.2%) less. These are sequential sets,
not randomized interleaving. The useful new behavior is that the engine
can now be loaded from fresh AOT, not an isolated additional arithmetic
kernel speedup.

A same-binary control with LATX_XTTS_AOT_UNMAP=0 and all hotspot switches
still on used its own empty HOME and the corrected writer wait. Evidence:
`/root/xzy/aikit-xtts-hot-20261015/validate-unmap-v3-off-015827`.
Fresh JIT/generation/load: 9.449 / 9.986 / 4.588 seconds. Five subsequent
runs: 4.440, 4.408, 4.440, 4.432 and 4.461 seconds, mean 4.4362,
median 4.440. All PCM matched. With the option enabled, the corresponding
five-run mean is 2.9900 seconds (32.6% less). The same-binary control
supports the engine-cache improvement without relying on old binaries.

## Limits

Sub-five-second performance is established for complete execution with a
fully generated cache from this binary, including first loading that cache.
It is NOT established for an empty cache: that execution and its detached
generation remain slower. No other version's cache is required or used.
This snapshot targets the supplied SDK's fixed mapping and is not a
general dlclose caching policy. Generation can be skipped on exclusive
timeout. Repeated unload/reload, other SDK versions, nondefault FP modes,
asynchronous signal reconstruction and all invalid-memory cases inside
fused loops have not been exhaustively tested. Prior sporadic initialization
timeouts have not been proved fixed merely by these passing repetitions.
The option remains off by default.
