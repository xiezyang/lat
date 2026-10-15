# XTTS JSON String Loops

## Implementation

`LATX_XTTS_JSON=1` enables two SDK-specific direct IR2 replacements in
`libaikit.so`'s local `parse_string` function. The option is off by default.
Only two fixed guest entry addresses are considered; function-byte hashes
are checked at translation, not at every execution or at unrelated TBs.

- `0x550843e24e`: scan consecutive ordinary bytes while keeping the pointer
  in a register. Check the original unsigned content-relative length before
  each subsequent load. Stop at quote, backslash or the length boundary.
- `0x550843e34b`: copy consecutive non-backslash bytes with register-held
  source and destination pointers. Preserve the original bytewise order and
  final defined register values. Do not introduce wider speculative loads.

Both paths write updated local pointers back and enter the original loop
condition, which restores architectural flags. Initial backslashes enter
the original escape handler with the original equal-CMP flags and EAX.
Unicode conversion, allocation, invalid-input handling and all other SDK
tasks remain in the original implementation. The new loops remove per-byte
stack traffic, EFLAGS calculations and TB transitions.

An initial candidate appended the ordinary conditional TB after the native
loop. Runtime TB linking patched a branch into the new loop and corrupted
the source pointer. Moving register allocation alone did not fix it. The
kept implementation replaces each entire TB and dispatches explicitly to
the original continuation instead. The failed binaries are not passing
checkpoints.

## Validation

Build host: `yuerengan@192.168.8.39:12581`.
Board: `root@192.168.8.200`, idle before testing; tests ran sequentially.

The standalone test maps the supplied SDK and calls the complete original
`parse_string`, using guest allocation/free hooks. It records return status,
input offset, allocation size, free count, item type and the entire output
allocation including initialized unused bytes. Inputs end immediately before
an inaccessible page. Tests include 6,000 randomized or long inputs, valid
Unicode and surrogate pairs, malformed escapes and truncated inputs at every
position, with input offsets 0..3. Native x86 and LAT with the option off/on
matched byte-for-byte. Expanded output SHA256:
`af20b5f6828b5dd0e5d7bcc03f148e77b74d7620bddc3a43a5306463a980acd9`.
Board translation diagnostics confirmed both replacement entries were used.

Board binary: `/root/xzy/aikit-xtts-hot-20261015/latx-x86_64-json-v3`.
SHA256: `a95240c6f79941dce1542a6fabf0f262f521f2dd0009a136aecfc71c0436ceb5`.
Evidence: `/root/xzy/aikit-xtts-hot-20261015/validate-json-v3-001910`.

Actual SDK synthesis passed AOT-disabled execution, empty-cache generation,
loading the same binary's own new AOT and all measured PCM checks. PCM SHA256:
`d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.
Times: JIT 9.485 seconds, generation 9.920 seconds, first cache load 10.883
seconds, then warmups 5.745, 5.635 and 4.573 seconds. Five measured runs:
4.498, 4.548, 4.479, 4.504 and 4.509 seconds; mean 4.508 seconds, median
4.504 seconds. These are complete process wall times, not CPU-time sums.

Settings: LBT_X86=0, SOFTFPU unset, all preceding XTTS switches enabled,
LATX_TB_HASH_BITS=15 and the isolated `runtime-dns-lan` runtime. DNS settings
are unchanged from the preceding checkpoint. Results are not evidence of
sub-five-second first execution or performance with the original resolver.
Synchronous fault-state reconstruction inside the fused loops and all
possible SDK inputs are not exhaustively validated.

Twenty further complete executions took 4.524, 4.493, 4.486, 4.550, 4.510,
4.496, 4.563, 4.566, 4.596, 4.567, 4.886, 4.506, 4.481, 4.464, 4.528,
4.498, 4.495, 4.526, 4.516 and 4.519 seconds. Mean 4.5385 seconds,
median 4.5175 seconds, minimum 4.464 seconds, maximum 4.886 seconds;
none exceeded five seconds and every PCM matched. Relative to the previous
checkpoint's twenty-run mean of 4.6829 seconds, this is 0.1444 seconds
(3.1%) less wall time. The previous and current sets ran sequentially, not
as a randomized controlled comparison.

At 999 Hz user-cycle sampling, `perf-i8four.data` attributed 1.1804% to
the scan entry and 1.5450% to the copy entry (2.7254% combined).
`perf-json-v3.data` attributed 0.0309% and 0.0281% respectively (0.0590%
combined), with a matching newly written PCM. Counted periods across all
threads were used, rather than adding rounded report percentages. These
single-run proportions confirm a large local CPU reduction but are not
an exact wall-time attribution. Matrix, formatting and translation/AOT
recovery work remain visible in the new profile.

## Rejected Column-Matrix Experiment

An experimental column-indexed matrix kernel accumulated four selected
weight rows per scratch-array read/write, with an overlap guard and the
original one-row path as fallback. O1 and O2 each passed 111,000 normal
matrix comparisons. An additional 1,000 overlapping weight/scratch captures
matched the prior native header (SHA256
`93c403119f2a653c11bcb363e9041f9662409292f006171cb65a4edc103cfce8`).
Comparing overlapping inputs against the scalar C implementation exposed
a difference already present in the previous native header; this is not
claimed as full original-SDK alias validation.

Board evidence `validate-col4-002820` passed fresh JIT/AOT checks and every
PCM check. Twenty repeated executions averaged 4.5364 seconds, median
4.5115 seconds, range 4.484..4.663 seconds, all below five seconds.
The difference from JSON-only's 4.5385-second mean is 0.0021 seconds,
too small to establish a useful gain. The kernel experiment was removed.
The additional variable-row and native-header alias regression tests remain.
