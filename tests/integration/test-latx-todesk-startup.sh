#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 LATX_X86_64 OUTPUT_DIR" >&2
    exit 2
fi

latx=$(realpath "$1")
output_dir=$2
display=${DISPLAY:-:0}
xauthority=${XAUTHORITY:-/tmp/xauth_mgaXwt}
runtime_dir=${XDG_RUNTIME_DIR:-/run/user/$(id -u)}
dbus_address=${DBUS_SESSION_BUS_ADDRESS:-unix:path=$runtime_dir/bus}
window_id=
runner_pid=

mkdir -p "$output_dir"

cleanup()
{
    if [[ -n "$runner_pid" ]] && kill -0 "$runner_pid" 2>/dev/null; then
        kill "$runner_pid" 2>/dev/null || true
        wait "$runner_pid" 2>/dev/null || true
    fi
}
trap cleanup EXIT HUP INT TERM

env DISPLAY="$display" XAUTHORITY="$xauthority" xdpyinfo \
    >"$output_dir/xdpyinfo.txt"

if env DISPLAY="$display" XAUTHORITY="$xauthority" \
    xwininfo -root -tree | grep -q '"ToDesk": ("ToDesk" "ToDesk")'; then
    echo "FAIL a ToDesk window already exists before the test" >&2
    exit 1
fi

(
    cd /opt/todesk/bin
    timeout --foreground --signal=TERM --kill-after=5s 20s env \
        DISPLAY="$display" \
        XAUTHORITY="$xauthority" \
        XDG_RUNTIME_DIR="$runtime_dir" \
        DBUS_SESSION_BUS_ADDRESS="$dbus_address" \
        LIBVA_DRIVER_NAME=iHD \
        LIBVA_DRIVERS_PATH=/opt/todesk/bin \
        LATX_AVX_CPUID=0 \
        LATX_AVX_TRACE=0 \
        "$latx" /opt/todesk/bin/ToDesk
) >"$output_dir/todesk.log" 2>&1 &
runner_pid=$!

for _ in $(seq 1 15); do
    window_id=$(env DISPLAY="$display" XAUTHORITY="$xauthority" \
        xwininfo -root -tree | awk '
            /"ToDesk": \("ToDesk" "ToDesk"\)/ && /770x520/ {
                print $1
                exit
            }
        ')
    if [[ -n "$window_id" ]]; then
        break
    fi
    if ! kill -0 "$runner_pid" 2>/dev/null; then
        echo "FAIL ToDesk exited before creating its main window" >&2
        sed -n '1,160p' "$output_dir/todesk.log" >&2
        exit 1
    fi
    sleep 1
done

if [[ -z "$window_id" ]]; then
    echo "FAIL ToDesk main window did not appear" >&2
    exit 1
fi

env DISPLAY="$display" XAUTHORITY="$xauthority" \
    xprop -id "$window_id" _NET_WM_PID WM_CLASS _NET_WM_NAME WM_NAME \
    >"$output_dir/window-properties.txt"
env DISPLAY="$display" XAUTHORITY="$xauthority" \
    xwininfo -id "$window_id" >"$output_dir/window-info.txt"

if ! grep -q 'Map State: IsViewable' "$output_dir/window-info.txt"; then
    echo "FAIL ToDesk main window is not viewable" >&2
    exit 1
fi
if ! grep -q 'WM_CLASS(STRING) = "ToDesk", "ToDesk"' \
    "$output_dir/window-properties.txt"; then
    echo "FAIL ToDesk main window has an unexpected WM_CLASS" >&2
    exit 1
fi

todesk_pid=$(sed -n 's/^_NET_WM_PID(CARDINAL) = //p' \
    "$output_dir/window-properties.txt")
if [[ -z "$todesk_pid" ]] || ! kill -0 "$todesk_pid" 2>/dev/null; then
    echo "FAIL ToDesk window PID is not running" >&2
    exit 1
fi
ps -p "$todesk_pid" -o pid,ppid,stat,etime,%cpu,%mem,cmd \
    >"$output_dir/process.txt"

set +e
wait "$runner_pid"
runner_status=$?
set -e
runner_pid=
if [[ "$runner_status" -ne 124 && "$runner_status" -ne 137 ]]; then
    echo "FAIL bounded ToDesk run exited with status $runner_status" >&2
    sed -n '1,160p' "$output_dir/todesk.log" >&2
    exit 1
fi

for _ in $(seq 1 5); do
    if ! env DISPLAY="$display" XAUTHORITY="$xauthority" \
        xwininfo -root -tree | grep -q '"ToDesk": ("ToDesk" "ToDesk")'; then
        echo "PASS ToDesk main window was viewable and cleanup completed"
        sed -n '1,20p' "$output_dir/window-properties.txt"
        grep 'Map State:' "$output_dir/window-info.txt"
        exit 0
    fi
    sleep 1
done

echo "FAIL ToDesk window remains after bounded run" >&2
exit 1
