#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 LATX_X86_64 PROBE X86_REFERENCE" >&2
    exit 2
fi

latx=$1
probe=$2
reference=$3
reference_dir=$(CDPATH= cd -- "$(dirname "$reference")" && pwd)
script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

fault_cases='load-cross-1 load-cross-7 load-cross-unmasked'
fpe_cases='fpe-invalid fpe-denormal fpe-invalid-priority'
no_signal_cases='no-signal-qnan-suppresses-denormal no-signal-daz no-signal-old-sticky no-signal-invalid-suppresses-denormal'

[ -x "$latx" ] || { echo "FAIL LATX binary is not executable" >&2; exit 2; }
[ -x "$probe" ] || { echo "FAIL VUCOMISD probe is not executable" >&2; exit 2; }
[ -f "$reference" ] || { echo "FAIL x86 reference is not a file" >&2; exit 2; }

"$script_dir/check-latx-avx-single-mnemonic.sh" "$probe" vucomisd

option_address=$(nm -g "$latx" | awk '$3 == "option_enable_lasx" {
    print "0x" $1
}')
[ -n "$option_address" ] || {
    echo "FAIL option_enable_lasx symbol is missing" >&2
    exit 1
}

run_lasx()
{
    name=$1
    output=$2
    error=$3
    status_file=$4
    set +e
    env LATX_AVX_CPUID=1 LATX_AOT=0 "$latx" "$probe" "$name" \
        >"$output" 2>"$error"
    status=$?
    set -e
    printf '%s\n' "$status" >"$status_file"
}

run_lsx()
{
    name=$1
    output=$2
    error=$3
    status_file=$4
    gdb_log=$5
    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'handle SIGSEGV pass nostop noprint' \
        -ex 'handle SIGBUS pass nostop noprint' \
        -ex 'handle SIGFPE pass nostop noprint' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        -ex 'break translate_context_init' \
        -ex "run $probe $name > $output 2> $error" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue >"$gdb_log" 2>&1
    gdb_status=$?
    set -e
    [ "$gdb_status" -eq 0 ] || {
        echo "FAIL LSX GDB status=$gdb_status case=$name" >&2
        cat "$gdb_log" >&2
        exit 1
    }
    grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$gdb_log" || {
        echo "FAIL LSX option_enable_lasx was not 0 case=$name" >&2
        cat "$gdb_log" >&2
        exit 1
    }
    if grep -Fq 'exited normally' "$gdb_log"; then
        printf '0\n' >"$status_file"
    elif grep -Fq 'exited with code 0213' "$gdb_log"; then
        printf '139\n' >"$status_file"
    elif grep -Fq 'exited with code 0210' "$gdb_log"; then
        printf '136\n' >"$status_file"
    else
        echo "FAIL LSX exit status is missing case=$name" >&2
        cat "$gdb_log" >&2
        exit 1
    fi
}

check_status()
{
    name=$1
    status_file=$2
    if [ "$name" = reference ]; then
        expected_status=0
    else
        expected_status=$(tr -d '[:space:]' <"$reference_dir/$name.status")
    fi
    actual=$(tr -d '[:space:]' <"$status_file")
    [ "$actual" = "$expected_status" ] || {
        echo "FAIL $name status expected=$expected_status actual=$actual" >&2
        exit 1
    }
}

check_fault_record()
{
    name=$1
    actual=$2
    expected="$reference_dir/$name.native"
    [ "$(wc -c <"$actual")" -eq 64 ] || {
        echo "FAIL $name fault record size" >&2
        exit 1
    }
    header=$(od -An -v -tx4 -N8 "$actual" | tr -d ' \n')
    [ "$header" = 0000000b00000002 ] || {
        echo "FAIL $name fault signal/code: $header" >&2
        exit 1
    }
    offset=$(od -An -v -tu8 -j8 -N8 "$actual" | tr -d ' \n')
    [ "$offset" = 4096 ] || {
        echo "FAIL $name fault offset: $offset" >&2
        exit 1
    }
    cmp -n 40 "$expected" "$actual"
    compare_signal_rflags "$expected" "$actual" 40 "$name"
    cmp -i 48 "$expected" "$actual"
}

check_fpe_record()
{
    name=$1
    actual=$2
    expected="$reference_dir/$name.native"
    [ "$(wc -c <"$actual")" -eq 64 ] || {
        echo "FAIL $name FPE record size" >&2
        exit 1
    }
    signal_number=$(od -An -v -td4 -N4 "$actual" | tr -d ' \n')
    signal_code=$(od -An -v -td4 -j4 -N4 "$actual" | tr -d ' \n')
    case "$name" in
        fpe-invalid|fpe-invalid-priority) expected_code=7 ;;
        fpe-denormal) expected_code=5 ;;
        *) echo "FAIL unknown FPE case: $name" >&2; exit 1 ;;
    esac
    [ "$signal_number" = 8 ] && [ "$signal_code" = "$expected_code" ] || {
        echo "FAIL $name signal/code: $signal_number/$signal_code" >&2
        exit 1
    }
    signal_offset=$(od -An -v -td8 -j8 -N8 "$actual" | tr -d ' \n')
    rip_offset=$(od -An -v -td8 -j16 -N8 "$actual" | tr -d ' \n')
    [ "$signal_offset" = 0 ] && [ "$rip_offset" = 0 ] || {
        echo "FAIL $name signal/RIP offset: $signal_offset/$rip_offset" >&2
        exit 1
    }
    cmp -n 32 "$expected" "$actual"
    compare_signal_rflags "$expected" "$actual" 32 "$name"
    cmp -i 40 "$expected" "$actual"
}

compare_signal_rflags()
{
    expected=$1
    actual=$2
    offset=$3
    name=$4
    expected_flags=$(od -An -v -tu8 -j "$offset" -N8 "$expected" |
        tr -d ' \n')
    actual_flags=$(od -An -v -tu8 -j "$offset" -N8 "$actual" |
        tr -d ' \n')
    expected_flags=$((expected_flags & ~65536))
    actual_flags=$((actual_flags & ~65536))
    [ "$actual_flags" -eq "$expected_flags" ] || {
        echo "FAIL $name RFLAGS excluding signal-frame RF" >&2
        exit 1
    }
}

run_and_compare()
{
    name=$1
    if [ "$name" = reference ]; then
        expected=$reference
    else
        expected="$reference_dir/$name.native"
    fi
    lasx="$tmpdir/lasx-$name"
    lsx="$tmpdir/lsx-$name"
    run_lasx "$name" "$lasx.out" "$lasx.err" "$lasx.status"
    run_lsx "$name" "$lsx.out" "$lsx.err" "$lsx.status" "$lsx.gdb"
    check_status "$name" "$lasx.status"
    check_status "$name" "$lsx.status"
    case "$name" in
        load-cross-*)
            check_fault_record "$name" "$lasx.out"
            check_fault_record "$name" "$lsx.out"
            ;;
        fpe-*)
            check_fpe_record "$name" "$lasx.out"
            check_fpe_record "$name" "$lsx.out"
            ;;
        *)
            cmp "$expected" "$lasx.out"
            cmp "$expected" "$lsx.out"
            ;;
    esac
    printf 'PASS %s x86/LASX/LSX\n' "$name"
}

run_and_compare reference
for name in $fault_cases $fpe_cases $no_signal_cases; do
    run_and_compare "$name"
done

sha256sum \
    tests/integration/latx-avx-single-vucomisd.S \
    tests/integration/latx-avx-single-vucomisd.c \
    target/i386/latx/translator/tr-avx-cmp.c \
    "$probe" "$latx" "$reference"
echo 'PASS VUCOMISD full three-way differential; LATX_AOT=0'
