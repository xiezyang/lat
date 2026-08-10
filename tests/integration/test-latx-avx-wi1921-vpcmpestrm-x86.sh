#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 2 ]]; then echo "usage: $0 xzy86 OUTPUT_DIR" >&2; exit 2; fi
host=$1; out=$2; mnemonic=vpcmpestrm; stem="latx-avx-single-$mnemonic"
root=$(cd "$(dirname "$0")/../.." && pwd); mkdir -p "$out"
remote_dir="/tmp/wi1921-$mnemonic-$(date +%s)-$$"
ssh_args=(); scp_args=();
if [[ -n "${LATX_SSH_CONFIG:-}" ]]; then ssh_args=(-F "$LATX_SSH_CONFIG"); scp_args=(-F "$LATX_SSH_CONFIG"); fi
ssh "${ssh_args[@]}" "$host" mkdir -p "$remote_dir"
scp "${scp_args[@]}" "$root/tests/integration/latx-avx-single-runtime.S" "$root/tests/integration/$stem.S" "$root/tests/integration/$stem.c" "$root/tests/integration/latx-avx-single-common.h" "$host:$remote_dir/"
ssh "${ssh_args[@]}" "$host" bash -s -- "$remote_dir" "$stem" <<'REMOTE'
set -euo pipefail
remote_dir=$1; stem=$2
docker run --rm --env STEM="$stem" -v "$remote_dir:/work" -w /work latx-ci-baseline:latest bash -ceu '
  gcc -std=c11 -O0 -Wall -Wextra -Werror -nostdlib -static -no-pie -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone -mgeneral-regs-only -mno-avx -mno-avx2 -I /work -o "/work/$STEM.static" /work/latx-avx-single-runtime.S "/work/$STEM.S" "/work/$STEM.c"
  objdump -d -Mintel "/work/$STEM.static" > "/work/$STEM.objdump"
'
REMOTE
scp "${scp_args[@]}" "$host:$remote_dir/$stem.static" "$out/$stem.static"
scp "${scp_args[@]}" "$host:$remote_dir/$stem.objdump" "$out/$stem.objdump"
run_case() {
    local name=$1; set +e
    ssh "${ssh_args[@]}" "$host" "set +e; $remote_dir/$stem.static $name >$remote_dir/x86-$name.stdout 2>$remote_dir/x86-$name.stderr; printf '%s\\n' \$? >$remote_dir/x86-$name.exit; exit 0" >"$out/x86-$name.ssh.stdout" 2>"$out/x86-$name.ssh.stderr"
    scp "${scp_args[@]}" "$host:$remote_dir/x86-$name.stdout" "$out/x86-$name.stdout"
    scp "${scp_args[@]}" "$host:$remote_dir/x86-$name.stderr" "$out/x86-$name.stderr"
    scp "${scp_args[@]}" "$host:$remote_dir/x86-$name.exit" "$out/x86-$name.exit"
    set -e
}
run_case normal; run_case fault
[[ "$(cat "$out/x86-normal.exit")" == 0 ]]; [[ "$(cat "$out/x86-fault.exit")" == 139 ]]
sha256sum "$out/$stem.static" "$out/$stem.objdump" "$out/x86-normal.stdout" "$out/x86-normal.stderr" "$out/x86-fault.stdout" "$out/x86-fault.stderr" >"$out/sha256.txt"
cat >"$out/manifest.json" <<EOF
{"work_item":"WI-1921","mnemonic":"$mnemonic","baseline":"xzy86-native","normal_exit":0,"fault_exit":139,"normal_bytes":512,"fault_bytes":0,"objdump":"$out/$stem.objdump","sha256":"$out/sha256.txt","forms":["XMM register","m128 memory","alias","explicit/implicit length","EFLAGS","ECX index or XMM0 mask"]}
EOF
printf 'PASS WI-1921 xzy86 native fixture: %s\n' "$mnemonic"
