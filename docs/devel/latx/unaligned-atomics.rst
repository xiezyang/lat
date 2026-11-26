Cache-line-local unaligned atomics
==================================

``LATX_UNALIGNED_ATOMIC=1`` opts into hosts that guarantee the full atomic
semantics of AM* and LL/SC at unaligned addresses within a 64-byte cache line.
The default, including an unset variable, is ``0``. The equivalent command-line
option is ``-latx-unaligned-atomic 0|1``; configuration files accept the same
environment variable name. Values other than exactly ``0`` or ``1`` are errors.

This is an explicit hardware contract, not automatic CPU-model detection. LAT
also requires CPUCFG.2 LAM, LAM_BH and LAMCAS and rejects the opt-in if these
instructions are absent. Those instruction-availability bits alone do not
prove unaligned access support. Enabling this option on an unsupported host is
not a substitute for checking the hardware. Keep the setting fixed for each
host: enabled on supported hardware and disabled on older hardware.

Translation
-----------

The enabled mode uses the existing exact-width AM* translators and additionally
translates LOCK ADC/SBB with AMADD_DB.B/H/W/D. Their original source and incoming
carry are retained for x86 flag calculation, including carry/borrow wrapping and
SBB's AF calculation. LOCK NEG uses an exact-width AMCAS_DB retry loop; flags
are updated only after a successful attempt.

These paths do not round down the address or update a larger field to provoke
ALE. Cross-line accesses still use the exact x86 width and can enter the existing
SIGBUS interpreter when rejected by hardware. The option does not assert native
cross-line atomicity, change the memory-ordering requirements, or remove page
permission checks. Locked BTS/BTR/BTC use exact-width AM* operations (a CAS
loop for 16 bits), while retaining x86's signed bit-string address calculation.
CMPXCHG8B retains its exact-width LL.D/SC.D implementation.

When SC.Q is absent, the enabled CMPXCHG16B fallback emits the reserved software
BREAK code ``0x4c41`` before its AMSWAP.D marker. LAT routes that synchronous
SIGTRAP, only at a translated host PC, to the existing atomic interpreter. It
no longer assumes LL.D at address 4 must raise ALE. Other SIGTRAP signals retain
their normal handling. This changes the interpreter entry, not its existing
page-relocation algorithm or its concurrency guarantees.

With the option off, instruction selection and the legacy ALE-triggered
fallback remain unchanged. New hosts that relax alignment must use the enabled
mode: deliberately running their legacy fault-driven sequences can bypass the
interpreter.

Validation boundaries
---------------------

``lat-pr-fast`` includes three host-independent production-body tests::

    python3 tests/unit/test-latx-unaligned-atomic.py
    python3 tests/unit/test-latx-atomic-trap.py
    python3 tests/unit/test-latx-atomic-width.py

The first executes the emitted ADC/SBB/NEG/BTS/BTR/BTC IR in a byte-addressed
model, injects CAS conflicts, checks adjacent bytes and carry inputs, and rejects
invalid option values and absent instruction capabilities.
The second exercises both CMPXCHG16B fallback instruction sequences and the real
interpreter/dispatch bodies for success, mismatch and permission failure; it
also checks that SC.Q selection is unchanged.
The third executes the AMADD/AMAND/AMOR/AMXOR interpreter bodies with ordinary
NOPs and legacy 16/32-bit width markers, including builds with ``NDEBUG``.
Only the two defined marker encodings override the instruction width; an
ordinary NOP must not suppress an interpreted memory update.

These tests model host instruction execution, page relocation and parts of the
signal context. They do not prove LoongArch LBT flags, kernel signal delivery,
native non-aligned atomicity, or concurrency of the page-relocation fallback.
Target correctness and performance measurements remain separate requirements.

Native hardware probe
---------------------

Build this standalone program with a LoongArch Linux C compiler and run it
without LAT::

    cc -O2 -Wall -Wextra -Werror -pthread \
      tests/native/loongarch-unaligned-atomic.c -o native-unaligned-atomic
    ./native-unaligned-atomic

It sweeps every cache-line-local offset for the AM*_DB variants used by LAT
and LL.W/SC.W and LL.D/SC.D. It checks old values, updated values and adjacent
bytes, and includes both AMCAS comparison outcomes. Two threads pinned to
different available CPUs contend on AMADD, AMCAS and LL/SC for every natural
alignment residue. Returned-old-value histograms supplement the final counters,
including byte-counter wrapping.

SIGBUS, SIGSEGV or SIGILL during the sweep is reported as unsupported, never
interpreted as a successful access. Exit 0 means the checks passed; 77 means
an instruction/alignment or the two-CPU environment was unavailable; 1 means
an incorrect result or a setup failure. ``--require`` converts 77 to failure
when this hardware contract is expected. Uncached memory, cross-line native
atomicity and memory-ordering litmus tests are outside this probe.

The program uses raw encodings for v1.1 AM instructions so the assembler need
not recognize their mnemonics. The base LL/SC instructions use normal assembly.
The ``test-native-unaligned-atomic`` integration registration builds the probe
only when that test is run. To require the new hardware during suite execution::

    LATX_TEST_UNALIGNED_ATOMIC=1 meson test -C build64-tests \
      --suite latx-integration test-native-unaligned-atomic --print-errorlogs

A prebuilt probe from the tested checkout can be supplied with
``LATX_ATOMIC_NATIVE_PROBE=/absolute/path/to/native-unaligned-atomic`` when
a native compiler is unavailable. The runner records its SHA256; retain its
source revision and build command with the result.

LAT guest regression
--------------------

``tests/integration/unaligned-atomic.c`` is a freestanding Linux x86 fixture.
Its expected memory results and arithmetic flags are calculated independently
in C. It covers all in-line offsets and 8/16/32-bit operations (also 64-bit for
x86_64), carry/borrow and signed boundaries, XADD/XCHG returned values, both
CMPXCHG outcomes, CMPXCHG8B, signed and out-of-word bit indices, cross-line
accesses, adjacent-byte preservation, a shared counter across fork, and a
cross-page permission fault without a partial update. x86_64 also checks
aligned CMPXCHG16B success and mismatch.

The following commands run the mode-0 tests on legacy hardware::

    meson test -C build64-tests --suite latx-integration \
      test-unaligned-atomic-x86_64-0 test-unaligned-atomic-i386-0 \
      --print-errorlogs

On new hardware, opt into and run the mode-1 tests::

    LATX_TEST_UNALIGNED_ATOMIC=1 meson test -C build64-tests \
      --suite latx-integration \
      test-unaligned-atomic-x86_64-1 test-unaligned-atomic-i386-1 \
      --print-errorlogs

The enabled tests require the native probe to pass first. This prevents an old
host's SIGBUS interpreter from passing as new hardware support. Without the
explicit test opt-in the mode-1 tests skip; with it the legacy mode-0 tests
skip. On new hardware, the option must be enabled in the tested translator.

The runner builds fixtures only at test execution using Clang/LLD. To supply
fixtures built on an x86 Linux machine from the tested checkout::

    mkdir -p atomic-guests
    gcc -m64 -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin \
      -fno-stack-protector -fno-pie -fomit-frame-pointer -mno-red-zone \
      -nostdlib -static -no-pie -Wl,--build-id=none \
      tests/integration/unaligned-atomic.c \
      -o atomic-guests/unaligned-atomic-x86_64
    gcc -m32 -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin \
      -fno-stack-protector -fno-pie -fomit-frame-pointer \
      -nostdlib -static -no-pie -Wl,--build-id=none \
      tests/integration/unaligned-atomic.c \
      -o atomic-guests/unaligned-atomic-i386

Set ``LATX_ATOMIC_GUEST_DIR=/absolute/path/to/atomic-guests`` on the target.
A missing fixture is a failure. Alternatively ``LATX_ATOMIC_GUEST_CC`` selects
a cross compiler command; the runner adds ``-m32`` or ``-m64`` as appropriate.
The runner records fixture SHA256 values and requires the final PASS sentinel.
Timeout or abnormal termination fails and terminates the guest process group.

The tests explicitly set ``LATX_SOFTFPU=0`` and ``LATX_SOFTFPU_FAST=0`` in the
environment and command line; they exercise integer operations. AOT and KZT are
disabled. ``LATX_CLOSE_PARALLEL=1`` prevents the single-thread atomic shortcut.
``LATX_MT=2`` requests guest page checks on 16K hosts and is disabled by LAT on
4K hosts. Address-translation selection is not changed by these tests.

The script-driven fixtures are registered in ``latx-integration`` and are not
built by normal product builds. Neither a passing native probe nor a passing
host IR model substitutes for running these guests through the candidate LAT.
Weak-memory ordering, uncached accesses and concurrent cross-line software
fallback remain separate validation gates.
