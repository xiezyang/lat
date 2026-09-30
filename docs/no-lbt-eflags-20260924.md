# no-LBT EFLAGS optimization, 2026-09-24

This experiment changes only translator/tr-eflag-process.c. Existing uncommitted
changes in other files are retained. The source snapshot was copied to xzyla at
/tmp/lat-eflags-opt-20260924 before additional concurrent AOT edits appeared in
the local working tree; those later edits are not covered by these results.

## Changes

- ZF: use unsigned compare with 1 and insert the Boolean directly.
- SF: extract the result sign bit and insert it directly.
- AF: omit masking that the existing single-bit writer already performs.
- CF: insert the computed low bit directly, without extracting bit zero again.
- OF: insert the computed Boolean without shifting it to bit 11 and back.
- Logical CF/OF: clear the flag directly rather than calculate a zero temporary.
- MUL/IMUL: calculate overflow once when CF and OF are both needed.

Generated instruction counts from the source paths, excluding operand loads:

| Operation | Before | After |
| --- | ---: | ---: |
| ZF, 64-bit | 4 | 2 |
| ZF, narrower operand | 5 | 3 |
| SF, wider than 8 bits | 5 | 2 |
| SF, 8-bit | 4 | 2 |
| Logical CF | 3 | 1 |
| Logical OF | 5 | 1 |

These are local sequence counts, not measured whole-program code-size savings.

## Validation

211 SSH at 192.168.8.211:12581 returned No route to host. Compilation used xzyla,
not 211. GNU ld 2.44 failed on static PIE; static non-PIE linking succeeded with
the missing libzstd static dependency unpacked into the experiment directory.
Both baseline and optimized builds used the same objects and linking command,
except for tr-eflag-process.c. O1, static, no KZT.

The existing latx-eflags-differential.S from
/home/xzy/work/lat-spec-no-lbt-20260916/tests/integration was compiled as a static
x86-64 guest with 4096 iterations per family. The 80-byte output matched between
baseline and optimized binaries on xzyla. On board 200, optimized output also
matched the previous /tmp/lat-eflags-200-20260922-v2/latx-x86_64 binary.

Output SHA256:
13d277b760a586ce605fdf2280e1d1e24b22f68d5569f0965759c51da7fe48a6

LATX_AOT=1 and LATX_SOFTFPU unset throughout. This establishes agreement with
the baseline, not exhaustive architectural correctness or AOT cache-load coverage.

## Remaining Blocker

XFYun on board 200 crashed with both freshly compiled baseline and optimized
binaries, including separate initially empty AOT cache directories. Baseline
failed after about 1.48 seconds and optimized after 1.41 seconds; these are
failure times and MUST NOT be interpreted as performance measurements.
The toolchain/environment cause is not yet established. No valid XFYun speedup
or total translated-code reduction is claimed. Prior board binaries were not
replaced. Experimental binaries reside in /tmp on board 200.

Next: compile both revisions with the known-working 211 toolchain, repeat the
flag differential test and XFYun with separate newly generated AOT caches, check
cache loading and WAV hashes, then compare warm medians and generated code size.

## Follow-up: Original 211 Moved to 192.168.8.23

Local x86-64 verification used qemu-loongarch64 to execute both earlier
LoongArch binaries. Both produced the same 80-byte differential output and hash
above, with AOT enabled and LATX_SOFTFPU unset.

The current working tree, including concurrent AOT changes, was then copied to
/home/yuerengan/xzy/eflags-bitopt-20260924 on 192.168.8.23:12581. O1 static no-KZT
compilation succeeded using the established Meson from
/home/yuerengan/xzy/lat-master-aot-debug-20260919/meson/meson.py.
The native differential test passed with the same hash.

Binary SHA256:
4d5c5a0f8fc20c10f3b8582a1d56fef375e866aa432c3f11899f606616dea037

Board 200 binary: /tmp/lat-eflags-opt-built23-20260924.
Logs and separate per-version AOT caches:
/tmp/eflags-built23-perf-20260924.

XFYun completed successfully in all three optimized runs: 9.81 seconds on the
initial cache-empty run, then 7.22 and 7.39 seconds with 10 AOT segments loaded.
All WAV hashes matched
e0f9aeaf8619a87fa510ba8891138aa0bc19e1dd1d2d10c72b9c428cdb21348a.
The historical binary ran in 13.94, 9.75 and 5.43 seconds in interleaved runs.
This noisy comparison also includes other source changes; it does not isolate
EFLAGS speedup and must not be compared directly to the previous 4.34s median.
The earlier XFYun execution blocker is resolved with the established toolchain.
An otherwise identical baseline and controlled timing/code-size comparison are
still required to quantify this patch's benefit.

## Comparison With origin/no-lbt-x86_64-jit

Fetched e58ac5dec3 and checked out /home/xzy86/work/lat-jit-compare-20260924.
Built on 192.168.8.23 with the same O1/static/no-KZT configure arguments.
Remote source: /home/yuerengan/xzy/lat-jit-compare-20260924.
Board binary: /tmp/lat-jit-e58ac5-20260924.
SHA256: 4521f817f64c00a4b84ca31320f35583ffb5a1945e2f19f0ae62cb599138ef76.

Important configuration difference: the remote branch automatically sets
option_softfpu=2 in no-LBT mode. Its handle_arg_latx_aot still disables AOT if
option_softfpu is nonzero. Explicit LATX_AOT=1 therefore disabled AOT in this
experiment (zero cache files; roughly 10s runs). Unsetting LATX_AOT allowed its
default-enabled AOT to generate 11 cache files and reach the expected speed.
Both measured versions used default-enabled AOT, unset LATX_SOFTFPU, and unset
LATX_AOT_SCAN/LATX_TU_SCAN/LATX_NO_LBT_SCAN. Internal softfpu settings differ.

After warmup/cache generation, six alternating runs on board 200:

| Version | Seconds | Median |
| --- | --- | --- |
| Current | 4.40, 4.28, 4.19, 4.19, 4.25, 4.11 | 4.220 |
| Remote JIT branch | 4.81, 4.99, 4.45, 4.78, 4.63, 4.49 | 4.705 |

Current elapsed time is 10.3% lower (1.115x). All WAV hashes match the reference.
This is a whole-version comparison, not an isolated EFLAGS effect. The earlier
7s results included diagnostics and unstable runs and are not a stable warm
baseline. Diagnostics alone have not been proven to explain the entire gap.
Logs, WAVs and per-version caches: /tmp/jit-compare-20260924 on board 200.
Runner: /tmp/run-jit-compare-20260924.sh on local host and board 200.
