# FCVT no-LBT repair checkpoint

**Current status: implementation and focused validation completed on la-dev.**
The chronological notes below retain earlier failures; see the final result at
the end for the tested binary and remaining limits.

Base: 74201754f23267f4f387bfec7ccff7ff2c1af5fe, branch codex/no-lbt-recovery-20260919.

Existing ROL/ROR changes in tr-logic.c and latx-config-regression.c predate this repair; preserve them.

Completed source edit: tr-softfpu.c now requires option_enable_lbt for all SOFTFPU_FAST branches and the FLD/FILD inline conversion branches. Without LBT these use the existing software helper alternatives. This covers all 17 raw FCVT emitters in that file by their callers.

The six tr-opnd-process.c FCVT emitters now call guarded latx_fcvt_* functions in translate.c. Their software path uses SoftFloat with a host-stack frame saving all GPRs, vector registers, FCSR and FCC registers. A helper relocation was added. This path still requires focused validation for rounding, NaNs, unmasked exceptions, register preservation and AOT; do not treat source completeness as validation completeness.

No-LBT mode now selects SOFTFPU=2 when requested mode is 0: the old register-backed x87 path's tr_fpu_inc/dec simply return without maintaining the stack when LBT is disabled. Mode 1 and mode 2 remain selectable.

## Latest evidence (2026-09-22)

- 211 static build passed. Candidate on former test host: /tmp/latx-fcvt-candidate2-20260921; SHA256 a392d6bd7d195942421817d54971fa0a0731b94d16a6a5ef8ed03988e99185cd. No temporary fprintf instrumentation remains in source or this candidate.
- Native x86 matrix reference SHA256: 7bb7722a4955dcbfeb321ceafb3a0293c799ebbdaa53341089b05161d0cc72ff. The matrix with per-case write syscalls passed all six mode/FAST combinations on 200. This proves these separated cases, not uninterrupted regions.
- xfyun completed with rc=0; generated /tmp/latx-fcvt-20260921.wav SHA256 e0f9aeaf8619a87fa510ba8891138aa0bc19e1dd1d2d10c72b9c428cdb21348a, equal to existing reference.
- IMPORTANT NEGATIVE: compile fcvt-no-lbt-matrix.S with -DFCVT_CONTIGUOUS to omit per-case syscalls. Native output hash d55fbef695982ffbcb984bca32a839f3232b1a345ff70e888310d809ec6648bd; both baseline and candidate exit 139 under SOFTFPU=1 on 200. Same candidate failure on 211. This pre-existing failure remains unresolved and must not be hidden by the separated matrix.
- IMPORTANT NEGATIVE: fcvt-no-lbt.S after adding a write of its 32-byte data record fails self-check on 200 in modes 0/1/2. Mode 2 works on 211 with the same candidate and guest, suggesting an additional host-dependent issue. Need actual-path diagnosis. Earlier pre-write smoke version passed modes 1/2; do not conflate these fixture revisions.
- Baseline mode 2 pre-write smoke hung and was killed by timeout -k; do not describe this as confirmed SIGILL without a faulting-PC trace.
- Test fixtures and runner are added; runner registered in process/meson.build. Full registered test suite is NOT passed (tests disabled in existing build). Current smoke gate correctly exposes the remaining failure.

## Latest user direction

User restored quota and requested continuation. User then explicitly replaced testing on 200 with testing on xzyla: DO NOT run more tests on 200. Local SSH alias xzyla is absent and direct connection fails. Async clarification pending whether xzyla means la-dev (xzy@192.168.8.45:22522, hostname xzy-pc, 3A6000). la-dev read-only identification succeeded. Do not assume the aliases identify the same host without confirmation. xzy86 remains the x86 compilation host; 211 remains the static LAT compilation host.

## Update: master comparison and remaining work

User confirmed xzyla == la-dev. All tests after that direction ran on la-dev, not 200. User additionally requested: if master also fails, record the failure and stop investigating it.

Built exact local master 97429a33e82e0957baac261f1ec2ba343e599700 from git archive in /home/yuerengan/xzy/lat-master-fcvt-20260922.gEr2e6/build64 on 211, statically, O1. Master binary SHA256 39afa1555d40b87a7d7d16a11581f6669a7b216e8243ba694d1ba9fe9652fddd.

la-dev working directory: /tmp/latx-fcvt-20260922.Nbb4Rt. Candidate before IR2 index fix: latx-fcvt-final-20260922, SHA256 03c3854a1e61e82c3713d0da9aebfeb64f880ec6978f064d79d5b57225132cda. This contains the AOT footer invalidation suffix no-lbt-fcvt1 and final options normalization; it is not yet the final deliverable.

- On la-dev, smoke and separated matrix pass all six mode/FAST combinations, with a separate fresh guest pathname per run. LATX_AOT_SCAN showed generation, no prior cache load. Updated runner supports prebuilt guests via LATX_FCVT_MATRIX_GUEST / LATX_FCVT_SMOKE_GUEST because la-dev lacks clang/lld.
- GDB primitive-helper test passed exact 64/80-bit roundtrip and nearest/upward tie rounding including FCSR inexact bits. GDB generated-code test forced the register-backed mode only after initialization to reach the new primitive emitters; all three conversions passed, preserving r8/r12, xmm0/xmm15 and CF. Fixtures and GDB command files are retained under tests/integration/fcvt-no-lbt-{helper,registers}.*. This override is diagnostic only, not a supported mode claim.
- Master passes the continuous matrix, native hash d55fbef695982ffbcb984bca32a839f3232b1a345ff70e888310d809ec6648bd. Candidate crashes in label_dispose -> ir2_opcode before executing generated code. IR2 indices/links are signed 16-bit; no-LBT expansions exceed 32767. Faulting pointer is precisely array_base - 32768 * sizeof(IR2_INST), matching signed index wrap. Just changed _id/_prev/_next in include/ir2.h to int32. Rebuild and re-run continuous matrix next; do not claim this additional fix validated yet.
- New values fixture covers signed zero, finite values, subnormals, infinities, qNaN/sNaN and x87 status. Numeric results match native. Native full-record hash b4b5c1cd16c9179f0a8bbca2c6e4f7a30c7cb6347dca310e68599ac0991526ab. Both master and candidate hash 20cc8071d132abfdb90a6bcd3c79c8febc90f07e725c82f9a31c3cf5019e66cb: only missing x87 denormal-operand flag (bit 1) for the two subnormal inputs. Per user instruction, RECORD ONLY; do not fix it.
- No further 200 tests authorized. The older 200-only smoke discrepancy remains historical evidence; la-dev and 211 passed that fixture. Do not claim an established cause for the host difference.

Remaining mandatory work: rebuild IR2 width fix on 211; run fresh continuous matrix and registered focused runner on la-dev; repeat helper/register checks on final binary; record final binary SHA and tests. Full lat-pr-fast / complete integration suite have not run; no PR/push requested. Keep source edits and prior ROL/ROR changes uncommitted unless asked otherwise.

## Final focused result (2026-09-22)

The remaining work listed in the preceding chronological entry is completed.

- Final static binary on la-dev: `/tmp/latx-fcvt-20260922.Nbb4Rt/latx-fcvt-checked-20260922`.
- SHA256: `78e81ca74ce39011b69f883da039a7f6e9609d8a3159bcefd13a41c81fa38724`.
- Same binary built on 211 at `/home/yuerengan/xzy/lat-no-lbt-o1-performance-20260919/build64/latx-x86_64`; local copy `/tmp/latx-fcvt-checked-20260922`.
- `file` confirms static linking; ELF program headers have no INTERP entry.
- Root cause of continuous-block crash confirmed in GDB: 264966 IR2 instructions, array base `0x7fffb4630010`, faulting pointer `0x7fffb44d0010` (exactly base minus 32768 times old 44-byte IR2_INST size). These are diagnostic observations, not hard-coded product addresses. Widening the three signed indices to int32 fixes the reproduced crash.
- Focused runner passed 18 executions: smoke 64/80 roundtrip, separated arithmetic matrix, and continuous matrix, each with modes 0/1/2 and FAST 0/0xffffff. All paths are fresh copies per configuration. In no-LBT operation, requested mode 0 is normalized to mode 2, including after user configuration overrides.
- Repeated both GDB helper and generated-register-preservation tests with the FINAL binary: passed. Their diagnostic override reaches the primitive FCVT emitters even though normal no-LBT mode uses the complete software x87 stack.
- `git diff --check` and shell syntax check passed. Tests are registered in the integration process domain. Existing build has tests disabled; the script was run directly with prebuilt x86 guests from xzy86. Full lat-pr-fast and full integration suites were not run.
- User-approved master-equivalent limit: the two subnormal operands omit the x87 denormal-operand flag on BOTH exact master and this candidate; the numeric values and other records agree. This is recorded, not repaired.
- Earlier xfyun result was obtained before IR2-width and final option/cache changes; it passed with identical WAV. It was not rerun on 200 after the user changed the test host. Do not present it as a final-binary 200 test.
- Unmasked exception delivery, i386 guest builds and direct primitive-helper AOT relocation were not separately validated. The AOT footer was changed to reject artifacts generated before this FCVT repair.

Reproduction on la-dev, from the working directory above (copy the current script from this tree first):

```sh
LATX_FCVT_MATRIX_GUEST=./latx-fcvt-matrix-20260921 \
LATX_FCVT_SMOKE_GUEST=./latx-fcvt-no-lbt-20260921 \
LATX_FCVT_CONTIGUOUS_GUEST=./latx-fcvt-contiguous-20260921 \
./test-fcvt-no-lbt.sh ./latx-fcvt-checked-20260922 \
  ./fcvt-no-lbt-matrix.S ./fcvt-no-lbt.S
```

The primitive diagnostic scripts are `tests/integration/fcvt-no-lbt-helper.gdb`
and `fcvt-no-lbt-registers.gdb`; compile `fcvt-no-lbt-registers.S` statically on
xzy86. Use a fresh guest pathname and `gdb -batch -x SCRIPT --args BINARY
-latx-host-hwcap 0x10 GUEST`. The register test is intentionally not a native
x87-stack conformance test: it observes physical register conversion operations.

All changes remain in the isolated worktree, uncommitted and unpushed. No edits
were made to the user's dirty master checkout.

Validation plan: static x86 guest fixtures built on xzy86; native reference; unchanged baseline versus candidate on the no-LBT board; LATX_SOFTFPU=0,1,2 and FAST=0/all; FLD/FILD/FST/FIST and arithmetic callers; rerun xfyun. Build LAT statically on the configured 211 build host. No PR or push requested.

Build tree: /home/yuerengan/xzy/lat-no-lbt-o1-performance-20260919/build64 on the 211 host (SSH port 12581). Previous candidate on the 200 host: /tmp/latx-no-lbt-rol-ror-20260921/latx-x86_64. x86 compilation host: SSH alias xzy86. Credentials are in the conversation, not this file.

Timing correction: earlier 4.460 s was one /proc/uptime measurement. The attribution of the user's 8.068 s to cold caches was not established by a controlled experiment; do not repeat it as fact.
