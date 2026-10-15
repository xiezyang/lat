# XTTS Native sinf Validation, 2026-10-15

## Saved Candidate

`LATX_XTTS_SINF=1` enables a complete direct-IR2 replacement for the supplied
SDK's `sinf_c` entry at guest address `0x551818a770`. It is disabled by default
and restricted to no-LBT, 64-bit translation. The original 203-byte function
signature must match. Normal inputs with absolute-value bits in
`[0x00800000, 0x49800000)` use the replacement; other inputs retain ordinary
translation. The replacement preserves the original operation order rather
than using host sinf or fused multiply-add instructions.

## Validation

- Compilation host: `yuerengan@192.168.8.39:12581`.
- Board binary: `/root/xzy/aikit-xtts-hot-20261015/latx-x86_64-sinf-v3`.
- SDK runtime: `/root/xzy/aikit-board200-20261015/x86-test-root`.
- Evidence: `/root/xzy/aikit-xtts-hot-20261015/validate-sinf-211910`.
- `test-xtts-sinf-guest.c` captures five GPRs, defined arithmetic flags and all
  128 bits of XMM0 through XMM3. The 20,000 normal-input results match native
  x86 execution byte for byte with AOT disabled on the board.
- Eighteen additional boundary and special inputs produce identical output
  with the replacement enabled and disabled. Infinity results differ in NaN
  sign from native x86 in the existing ordinary translation path. This change
  does not fix that pre-existing difference.
- Fresh JIT, empty-cache AOT generation, loading that new cache and eight
  subsequent invocations all exited normally and produced matching new PCM.
  Previous output was moved aside before each run; no old AOT was copied.
- PCM SHA256:
  `d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac`.

| Phase | Wall Seconds |
| --- | ---: |
| AOT disabled | 14.714 |
| Empty-cache generation | 16.943 |
| First load, AOT diagnostics enabled | 15.969 |
| Warmup 1 | 16.046 |
| Warmup 2 | 7.513 |
| Warmup 3 | 6.568 |
| Measured 1 | 17.709 |
| Measured 2 | 8.803 |
| Measured 3 | 6.551 |
| Measured 4 | 6.481 |
| Measured 5 | 8.683 |

Mean: **9.645s**; median: **8.683s**. User CPU times in the five measured
runs were 6.661, 6.701, 6.759, 6.791 and 6.761 seconds. These results do not
establish a stable end-to-end speedup or meet the five-second objective.

## Remaining Limits

A separate slow strace run recorded two five-second DNS polling timeouts
against `1.1.1.1`, followed by a successful response from `8.8.8.8`. After
`AEE_End`, the main thread waited about 7.18 seconds for SDK network shutdown.
The trace is `/root/xzy/aikit-xtts-hot-20261015/wait-sinf-4.strace`.
This explains that traced delay, not necessarily every timing outlier. No DNS
configuration, authorization or SDK cleanup was disabled by this change.

Coverage is limited to the supplied SDK and current floating-point mode.
Alternate MXCSR modes, exceptional memory faults and all TB recovery/linking
cases have not been exhaustively verified. Keep the replacement opt-in.
