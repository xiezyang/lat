# XTTS Network Exit Delay, 2026-10-16

## Conclusion

The large whole-process variation is not in LAT translation or synthesis.
It is in the SDK's synchronous network work after `AEE_End`. The delay is
present on both board 200 and compile host 39, so it is not specific to the
board CPU or guest-emulation environment.

## Measurement Method

The same `latx-x86_64-next9`, SDK, guest runtime, AOT options and PCM check
were used on both machines. The run starts a timer, executes `aikit_test`,
then reads the timestamp embedded in the `AEE_End` log line. The difference
from process start to that timestamp is `pre_AEE`; the difference from that
timestamp to the actual process exit is `post_AEE`.

The normal stdout log is buffered, so waiting until `AEE_End` becomes visible
in the output file is not valid. The calculations use the timestamp written
inside the log line, not its file-arrival time.

## Board 200, Ten Runs

| Run | Total | Pre AEE_End | Post AEE_End |
| ---: | ---: | ---: | ---: |
| 1 | 2.889s | 2.712s | 0.177s |
| 2 | 2.941s | 2.761s | 0.180s |
| 3 | 2.928s | 2.748s | 0.180s |
| 4 | 2.916s | 2.735s | 0.182s |
| 5 | 2.952s | 2.779s | 0.174s |
| 6 | 2.912s | 2.737s | 0.175s |
| 7 | 2.927s | 2.748s | 0.179s |
| 8 | 2.910s | 2.721s | 0.190s |
| 9 | 2.954s | 2.780s | 0.174s |
| 10 | 2.915s | 2.734s | 0.181s |

All ten runs produced the expected PCM. When the post-end network request
happened to complete quickly, the complete board process was reproducibly
about 2.9 seconds.

Earlier board slow runs showed the same endpoint behavior with a much larger
post-end interval. One six-run network-only trace contained a run with a
3.830-second `ppoll`, a 1.506-second worker wait, and final event-log traffic
to `123.57.26.93:80`. The process did not exit until that work completed.

## Compile Host 39, Eight Runs

| Run | Total | Pre AEE_End | Post AEE_End |
| ---: | ---: | ---: | ---: |
| 1 | 1.281s | 0.612s | 0.669s |
| 2 | 1.367s | 0.619s | 0.748s |
| 3 | 4.317s | 0.621s | 3.696s |
| 4 | 1.313s | 0.610s | 0.703s |
| 5 | 1.336s | 0.613s | 0.723s |
| 6 | 1.290s | 0.613s | 0.677s |
| 7 | 1.315s | 0.616s | 0.699s |
| 8 | 1.329s | 0.610s | 0.719s |

The compile host has much lower pre-end time because of its faster CPU, but
it still has a 3.696-second `AEE_End`-to-exit delay in one run. This rules out
the board's CPU and LAT translation as the source of that outlier.

## Native x86, No LAT

The supplied x86-64 SDK was also run directly on the x86-64 development host
with its original `libaikit.so`, using the loader directly because the test
file is not marked executable:

```sh
/lib64/ld-linux-x86-64.so.2 --library-path ./libs ./aikit_test
```

Eight native runs produced:

| Run | Total | Pre AEE_End | Post AEE_End |
| ---: | ---: | ---: | ---: |
| 1 | 1.035s | 0.630s | 0.405s |
| 2 | 0.634s | 0.059s | 0.575s |
| 3 | 1.726s | 0.058s | 1.668s |
| 4 | 2.524s | 0.059s | 2.464s |
| 5 | 0.865s | 0.058s | 0.806s |
| 6 | 0.618s | 0.059s | 0.559s |
| 7 | 2.488s | 0.058s | 2.431s |
| 8 | 2.394s | 0.058s | 2.336s |

No LAT translator or guest emulation was involved in these runs. The SDK still
spent 0.4--2.46 seconds after the internal `AEE_End` timestamp, with the same
`genCurrentTime` network logs. Therefore LAT is not the cause.

The test source does not sleep or perform an intentional network wait after
`AIKIT_End`. After that call it closes the output file, deletes the two
builders, calls `AIKIT_UnInit()`, and returns. The observed post-end interval
therefore belongs to SDK cleanup/uninitialization or its network worker, not
to an explicit wait written in the test program.

## Network Contribution

The delayed paths call the SDK's `ConnectPool::syncRequest` and
`ConnectPool::xlogSyncRequest`. Traces use these endpoints:

* `114.118.65.16:443`
* `123.57.26.93:80`
* `123.56.17.44:80`
* `101.200.180.128:80`
* `180.76.76.76:80`

The `genCurrentTime setlocale fail` messages are logging warnings from the
same network path; they are not the cause. The problem is that a synchronous
event/network request remains on the process-exit path. Remote response time
or retry time is therefore included in the measured complete-process time.

This is a real SDK flow issue, independently of LAT: even when synthesis is
complete, process completion waits for post-`AEE_End` network work. A fix
should preserve reporting and cleanup while removing that request from the
synchronous exit path or applying a strict bounded wait.

## Offline Fast Verification

LAT tracing identified `isNetworkAvailable()` in `libaikit.so` as the source
of the repeated post-end probes. It tries `180.76.76.76:80` and
`223.5.5.5:80` with an approximately 200 ms `select` timeout, then additional
event-log/TLS work remains on the thread-join path.

`AIKIT_SetConfig("connect_timeout", ...)` and `retry_times` returned error
18500 for this SDK. Reducing only the hard-coded select timeout to 50 ms
helped some runs but did not remove the longer TLS/upload and join waits.

An offline-only library experiment made `isNetworkAvailable()` return false
immediately. This removes all post-end network work. Five warm LAT runs with
the normal x86 runtime and a fresh AOT cache took 2.845, 2.777, 2.759, 2.776
and 2.778 seconds. All produced the expected PCM. The same run with the
normal library could spend several seconds after `AEE_End`.

This patch disables online authorization and network event reporting. It is
valid only for a customer package with an already valid, matching offline
license. It must not replace the network-enabled package for first-time
activation or online operation.
