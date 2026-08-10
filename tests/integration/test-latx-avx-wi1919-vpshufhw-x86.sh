#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 2 ]]; then
    echo "usage: $0 xzy86 OUTPUT_DIR" >&2
    exit 2
fi
host=$1
out=$2
mnemonic=vpshufhw
stem="latx-avx-single-$mnemonic"
root=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p "$out"
remote_dir="/tmp/wi1919-$mnemonic-$(date +%s)-$$"
ssh_args=()
scp_args=()
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then
    ssh_args=(-F "$LATX_SSH_CONFIG")
    scp_args=(-F "$LATX_SSH_CONFIG")
fi
ssh "${ssh_args[@]}" "$host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$root/tests/integration/latx-avx-single-runtime.S" \
    "$root/tests/integration/$stem.S" "$root/tests/integration/$stem.c" \
    "$root/tests/integration/latx-avx-single-common.h" "$host:$remote_dir/"
ssh "${ssh_args[@]}" "$host" bash -s -- "$remote_dir" "$stem" <<'REMOTE'
set -euo pipefail
remote_dir=$1
stem=$2
docker run --rm --env STEM="$stem" -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
  gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie \
      -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
      -mgeneral-regs-only -mno-avx -mno-avx2 -I /work \
      -o "/work/$STEM.static" /work/latx-avx-single-runtime.S \
      "/work/$STEM.S" "/work/$STEM.c"
  objdump -d -Mintel "/work/$STEM.static" > "/work/$STEM.objdump"
'
REMOTE
scp "${scp_args[@]}" "$host:$remote_dir/$stem.static" "$out/$stem.static"
scp "${scp_args[@]}" "$host:$remote_dir/$stem.objdump" "$out/$stem.objdump"
run_case() {
    local case_name=$1
    local out_name=$2
    set +e
    ssh "${ssh_args[@]}" "$host" "set +e; $remote_dir/$stem.static $case_name >$remote_dir/$out_name.stdout 2>$remote_dir/$out_name.stderr; printf '%s\\n' \$? >$remote_dir/$out_name.exit; exit 0" \
        >"$out/$out_name.ssh.stdout" 2>"$out/$out_name.ssh.stderr"
    scp "${scp_args[@]}" "$host:$remote_dir/$out_name.stdout" "$out/$out_name.stdout"
    scp "${scp_args[@]}" "$host:$remote_dir/$out_name.stderr" "$out/$out_name.stderr"
    scp "${scp_args[@]}" "$host:$remote_dir/$out_name.exit" "$out/$out_name.exit"
    local rc
    rc=$(cat "$out/$out_name.exit")
    set -e
    printf 'remote_native_exit=%s\n' "$rc" >"$out/$out_name.status"
}
run_case normal x86-normal
run_case fault x86-fault
[[ "$(cat "$out/x86-normal.exit")" == 0 ]]
[[ "$(cat "$out/x86-fault.exit")" == 139 ]]
sha256sum "$out/$stem.static" "$out/$stem.objdump" \
    "$out/x86-normal.stdout" "$out/x86-normal.stderr" \
    "$out/x86-fault.stdout" "$out/x86-fault.stderr" >"$out/sha256.txt"
cat >"$out/manifest.json" <<EOF
{
  "work_item": "WI-1919",
  "mnemonic": "$mnemonic",
  "baseline": "xzy86-native",
  "normal": {"command": "ssh $host $remote_dir/$stem.static normal", "exit": 0},
  "fault": {"command": "ssh $host $remote_dir/$stem.static fault", "expected_exit": 139},
  "objdump": "$out/$stem.objdump",
  "sha256": "$out/sha256.txt",
  "coverage": ["XMM", "YMM", "register", "memory", "alias", "VEX.128 high-half clear", "cross-page fault"]
}
EOF
printf 'PASS WI-1919 xzy86 native fixture: %s\n' "$mnemonic"
