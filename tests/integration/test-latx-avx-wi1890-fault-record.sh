#!/bin/sh
set -eu

if [ "$#" -ne 4 ]; then
    echo "usage: $0 LATX_X86_64 PROBE X86_EXPECTED_DIR OUTPUT_DIR" >&2
    exit 2
fi

latx=$1
probe=$2
expected=$3
output=$4
cases='load-cross-1 load-cross-7 store-cross-1 store-cross-7'

[ -x "$latx" ] || { echo "FAIL binary is not executable: $latx" >&2; exit 1; }
[ -x "$probe" ] || { echo "FAIL probe is not executable: $probe" >&2; exit 1; }
[ -d "$expected" ] || { echo "FAIL expected directory is missing: $expected" >&2; exit 1; }
command -v gdb >/dev/null 2>&1 || { echo "FAIL gdb is required" >&2; exit 1; }
command -v nm >/dev/null 2>&1 || { echo "FAIL nm is required" >&2; exit 1; }

mkdir -p "$output"
binary_sha=$(sha256sum "$latx" | awk '{print $1}')
probe_sha=$(sha256sum "$probe" | awk '{print $1}')
option_address=$(nm -g "$latx" | awk '$3 == "option_enable_lasx" { print "0x" $1; exit }')
[ -n "$option_address" ] || {
    echo "FAIL option_enable_lasx symbol is missing" >&2
    exit 1
}

{
    printf 'binary=%s\n' "$latx"
    printf 'binary_sha256=%s\n' "$binary_sha"
    printf 'probe=%s\n' "$probe"
    printf 'probe_sha256=%s\n' "$probe_sha"
    printf 'option_enable_lasx_address=%s\n' "$option_address"
    printf 'x86_expected_source=%s\n' "$expected"
    printf 'build=not run by this script\n'
} >"$output/run-metadata.txt"

check_fault_record()
{
    file=$1
    status_file=$2
    expected_status=$3
    actual_status=$(cat "$status_file")
    [ "$actual_status" = "$expected_status" ] || {
        echo "FAIL $(basename "$file") status=$actual_status expected=$expected_status" >&2
        return 1
    }
    [ "$(wc -c <"$file")" -eq 64 ] || {
        echo "FAIL $(basename "$file") size=$(wc -c <"$file") expected=64" >&2
        return 1
    }
    header=$(od -An -v -tx4 -N8 "$file" | tr -d ' \n')
    [ "$header" = 0000000b00000002 ] || {
        echo "FAIL $(basename "$file") signal/code=$header" >&2
        return 1
    }
    offset=$(od -An -v -tu8 -j8 -N8 "$file" | tr -d ' \n')
    [ "$offset" = 4096 ] || {
        echo "FAIL $(basename "$file") fault_offset=$offset" >&2
        return 1
    }
}

run_lasx()
{
    name=$1
    dir=$output/$name
    mkdir -p "$dir"
    printf 'env LATX_AVX_CPUID=1 LATX_AOT=0 %s %s %s > %s/lasx.stdout 2> %s/lasx.stderr\n' \
        "$latx" "$probe" "$name" "$dir" "$dir" >"$dir/lasx.command"
    set +e
    env LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" "$name" \
        >"$dir/lasx.stdout" 2>"$dir/lasx.stderr"
    status=$?
    set -e
    printf '%s\n' "$status" >"$dir/lasx.status"
    check_fault_record "$dir/lasx.stdout" "$dir/lasx.status" 139
}

run_lsx()
{
    name=$1
    dir=$output/$name
    printf '%s\n' \
        "gdb -q -batch $latx" \
        "  -ex set pagination off" \
        "  -ex handle SIGSEGV pass nostop noprint" \
        "  -ex handle SIGBUS pass nostop noprint" \
        "  -ex set environment LATX_AVX_CPUID 1" \
        "  -ex set environment LATX_AOT 0" \
        "  -ex break translate_context_init" \
        "  -ex run $probe $name > $dir/lsx.stdout 2> $dir/lsx.stderr" \
        "  -ex set {int}$option_address = 0" \
        "  -ex x/wd $option_address" \
        "  -ex continue" >"$dir/lsx.command"
    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        -ex 'break translate_context_init' \
        -ex "run $probe $name > $dir/lsx.stdout 2> $dir/lsx.stderr" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue >"$dir/lsx.gdb" 2>&1
    gdb_status=$?
    set -e
    [ "$gdb_status" -eq 0 ] || {
        echo "FAIL $name GDB status=$gdb_status" >&2
        return 1
    }
    grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$dir/lsx.gdb" || {
        echo "FAIL $name option_enable_lasx readback" >&2
        return 1
    }
    if grep -Fq 'exited with code 0213' "$dir/lsx.gdb"; then
        printf '139\n' >"$dir/lsx.status"
    elif grep -Fq 'exited normally' "$dir/lsx.gdb"; then
        printf '0\n' >"$dir/lsx.status"
    else
        echo "FAIL $name LSX guest exit status is missing" >&2
        return 1
    fi
    check_fault_record "$dir/lsx.stdout" "$dir/lsx.status" 139
}

failed=0
printf 'name\tx86_sha256\tlasx_sha256\tlsx_sha256\tx86_rc\tlasx_rc\tlsx_rc\tlsx_full_match\tlasx_full_match\n' \
    >"$output/results.tsv"

for name in $cases; do
    x86="$expected/$name-x86.bin"
    [ -f "$x86" ] || { echo "FAIL missing x86 record: $x86" >&2; exit 1; }
    run_lasx "$name" || failed=1
    run_lsx "$name" || failed=1
    dir=$output/$name
    x86_sha=$(sha256sum "$x86" | awk '{print $1}')
    lasx_sha=$(sha256sum "$dir/lasx.stdout" | awk '{print $1}')
    lsx_sha=$(sha256sum "$dir/lsx.stdout" | awk '{print $1}')
    lsx_match=FAIL
    lasx_match=FAIL
    if cmp -s "$x86" "$dir/lsx.stdout"; then
        lsx_match=PASS
    else
        echo "FAIL $name LSX full 64-byte record differs from x86" >&2
        failed=1
    fi
    if cmp -s "$x86" "$dir/lasx.stdout"; then
        lasx_match=PASS
    else
        echo "INFO $name LASX differs from x86; deviation retained only"
    fi
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$name" "$x86_sha" "$lasx_sha" "$lsx_sha" \
        "$(cat "$x86.status" 2>/dev/null || printf 139)" \
        "$(cat "$dir/lasx.status")" "$(cat "$dir/lsx.status")" \
        "$lsx_match" "$lasx_match" >>"$output/results.tsv"
done

sha256sum "$latx" "$probe" "$expected"/*-x86.bin \
    "$output"/*/lasx.stdout "$output"/*/lsx.stdout >"$output/SHA256SUMS"

[ "$failed" -eq 0 ] || exit 1
echo "PASS WI-1890 complete 64-byte LSX fault records"
