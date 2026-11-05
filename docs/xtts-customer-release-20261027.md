# XTTS No-LBT Customer Release, 2026-10-27

## Source

Branch: `codex/no-lbt-recovery-20260919`

The release starts from commit `db9b479980fce19de95cb5fb820d1bf5a7f69299`
and includes the validated XTTS hotspot and AOT changes committed after it.
The XTTS substitutions are specific to the supplied AIKIT engine and SDK
library versions. Their byte-hash checks fall back to ordinary translation
when the guest function does not match.

## Build

Build on a LoongArch host with the x86_64 linux-user target:

```sh
../source/configure \
  --target-list=x86_64-linux-user \
  --enable-latx \
  --disable-debug-info \
  --optimize-O1 \
  --static \
  --extra-ldflags=-ldl \
  --disable-docs
ninja -j16 latx-x86_64
```

KZT is not enabled. The resulting binary is statically linked for a
LoongArch64 Linux host.

## Runtime

The guest runtime needs the x86_64 loader, glibc and the SDK's normal library
dependencies. For a minimal runtime, also include:

```text
runtime/usr/lib/locale/en_US.utf8/LC_TIME
```

The supplied SDK calls `setlocale(LC_TIME, "en_US.UTF-8")` while creating its
HTTP date. Without this file, the SDK prints `genCurrentTime: setlocale fail`
and falls back to the process locale.

Keep the SDK shared libraries beside the test program:

```text
libs/libaikit.so
libs/ebd1bade4_v1025_aee.so
```

## Run

From the prepared SDK directory:

```sh
LD_LIBRARY_PATH=./libs ./latx-x86_64 -L ./runtime ./aikit_test
```

Only a writable `HOME` is otherwise required. LAT stores AOT data under
`$HOME/.cache/latx`. The first run generates translations; later runs load
the matching cache. Do not mix caches between different LAT binaries or
different hotspot options.

The following paths are enabled when no environment variable is set:

```text
MATRIX DECRYPT CLOCK_NANOSLEEP HOT_TU SINF EXPF POWF LOGF FILTER COSF
MPI JSON MEMSET ACCUMULATE MPI_TRIM AES AOT_UNMAP
```

Set `LATX_XTTS_<name>=0` only to disable one path for diagnosis. The optional
`MPI_AUX` paths remain opt-in with `LATX_XTTS_MPI_AUX=1`.

## Validation

The packaged release uses the supplied SDK dynamic libraries unchanged. On
the LoongArch test board, all successful runs returned `AIKIT_Start:0`,
`AIKIT_Write:0` and `AEE_End ret 0`. Every output matched:

```text
d3717f211a1bc14c29c5983b1e528dcb5ed40d2e09c0f5e1ed1264ed4d1f9aac
```

After time synchronization and AOT warm-up, ten normal complete-process runs
ranged from 3.611 to 4.049 seconds, with a 3.751-second mean and a
3.747-second median. One separate run took 13.647 seconds because the SDK
network worker remained active after `AEE_End`; this is an SDK/network exit
delay, not translation time.

The board must have the correct system time. During validation an unsynced
RTC initially produced SDK HTTP 403 signature failures. `systemd-timesyncd`
corrected the clock automatically after network access became available.
