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

``lat-pr-fast`` includes two host-independent production-body tests::

    python3 tests/unit/test-latx-unaligned-atomic.py
    python3 tests/unit/test-latx-atomic-trap.py

The first executes the emitted ADC/SBB/NEG/BTS/BTR/BTC IR in a byte-addressed
model, injects CAS conflicts, checks adjacent bytes and carry inputs, and rejects
invalid option values and absent instruction capabilities.
The second exercises both CMPXCHG16B fallback instruction sequences and the real
interpreter/dispatch bodies for success, mismatch and permission failure; it
also checks that SC.Q selection is unchanged.

These tests model host instruction execution, page relocation and parts of the
signal context. They do not prove LoongArch LBT flags, kernel signal delivery,
native non-aligned atomicity, or concurrency of the page-relocation fallback.
Target correctness and performance measurements remain separate requirements.
